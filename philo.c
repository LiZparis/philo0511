/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   philo.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lzhang2 <lzhang2@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/25 19:08:37 by lzhang2           #+#    #+#             */
/*   Updated: 2024/11/05 16:03:54 by lzhang2          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../Include/philosophers.h"

void	error_message(char *message)
{
	printf("%s", message);
	exit(1);
}

void	init_mutex(pthread_mutex_t *mutex)
{
	if (pthread_mutex_init(mutex, NULL) != 0)
		error_message("Error: pthread_mutex_init failed\n");
}


void	init_time(t_prog *prog)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	prog->start_time = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

unsigned long	get_time(t_prog *prog)
{
	struct timeval	tv;
	unsigned long	current_time;

	gettimeofday(&tv, NULL);
	current_time = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000)  - prog->start_time);
	return (current_time);
}

void	print_status(t_prog *prog, t_philo *philo, const char *status)
{
	pthread_mutex_lock(philo->print_lock);
	printf("%ld %d %s\n", get_time(prog), philo->id, status);
	pthread_mutex_unlock(philo->print_lock);
}

int check_if_dead(t_prog *prog)
{
   	for (int i = 0; i < prog->num_philos; i++)
	{
        pthread_mutex_lock(&prog->meal_lock);

        if (get_time(prog) - prog->philos[i].last_meal >= prog->time_to_die)
		{
        	print_status(prog, &prog->philos[i], "died");
            prog->dead_flag = 1;
            pthread_mutex_unlock(&prog->meal_lock);
            return 1;
        }
		pthread_mutex_unlock(&prog->meal_lock);
    }
    return 0; // 没有人死亡
}

int	check_if_all_ate(t_prog *prog)
{
	int	i;
	int	finished_eating;

	i = -1;
	finished_eating = 0;
	if (prog->philos[0].num_times_to_eat == -1)
		return (0);
	while (++i < prog->num_philos)
	{
		pthread_mutex_lock(prog->philos[i].meal_lock);
		if (prog->philos[i].meals_eaten >= prog->philos[i].num_times_to_eat)
			finished_eating++;
		pthread_mutex_unlock(prog->philos[i].meal_lock);
	}
	if (finished_eating == prog->num_philos)
	{
		pthread_mutex_lock(&prog->dead_lock);
		prog->dead_flag = 1;
		pthread_mutex_unlock(&prog->dead_lock);
		return (1);
	}
	return (0);
}
void *monitor_routine(void *prog_arg) {
    t_prog *prog = (t_prog *)prog_arg;
    int finished_philos = 0;

    while (1) {
        finished_philos = 0;
        for (int i = 0; i < prog->num_philos; i++) {
            pthread_mutex_lock(&prog->meal_lock);

            // 检查死亡时间
            if (get_time(prog) - prog->philos[i].last_meal >= prog->time_to_die) {
                print_status(prog, &prog->philos[i], "died");
                prog->dead_flag = 1;
                pthread_mutex_unlock(&prog->meal_lock);
                return NULL;
            }

            // 检查用餐次数
            if (prog->philos[i].meals_eaten <=prog->philos[i].num_times_to_eat) {
                finished_philos++;
            }

            pthread_mutex_unlock(&prog->meal_lock);
        }

        // 如果所有哲学家都达到用餐次数，结束监控线程
        if (finished_philos == prog->num_philos)
		{
            pthread_mutex_lock(&prog->dead_lock);
            prog->dead_flag = 1;  // 设置dead_flag防止其他哲学家继续操作
            pthread_mutex_unlock(&prog->dead_lock);
            break;
        }

        usleep(100);  // 稍微降低检查频率
    }
    return NULL;
}

void	*monitor(void *prog_arg)
{
	t_prog	*prog;

	prog = (t_prog *)prog_arg;
	while (1)
	{
		pthread_mutex_lock(&prog->dead_lock);
		if (prog->dead_flag)
		{
            pthread_mutex_unlock(&prog->dead_lock);
            break;
        }
        pthread_mutex_unlock(&prog->dead_lock);
        if (check_if_dead(prog) == 1 || check_if_all_ate(prog) == 1)
           {
			 pthread_mutex_unlock(&prog->meal_lock);
			 break;
		   } 
		   pthread_mutex_unlock(&prog->meal_lock);
       
	}
	 usleep(100); // 更频繁地检查
	return (NULL);
}
int check_death(t_philo *philo, t_prog *prog)
{
    if (get_time(prog) - philo->last_meal >= prog->time_to_die)
    {
        print_status(prog, philo, "died");
        return 1;  // 表示哲学家死了
    }
    return 0;  // 表示哲学家没有死
}
void	*philosopher_routine(void *philo_arg)
{
	t_philo	*philo;
	t_prog	*prog;
	
	philo = (t_philo *)philo_arg;
	
	prog = philo->prog;
	
	if(philo->id % 2 == 0)
		usleep(50);

	while (1)
	{
   		print_status(prog, philo, "is thinking");

    	pthread_mutex_lock(&prog->forks[philo->id - 1]);  // 锁定左叉子
		print_status(prog, philo, "has taken a fork");
    	pthread_mutex_lock(&prog->forks[philo->id % prog->num_philos]); // 锁定右叉子

		pthread_mutex_lock(&prog->meal_lock);
        philo->last_meal = get_time(prog);
        philo->meals_eaten++;
        pthread_mutex_unlock(&prog->meal_lock);

		print_status(prog, philo, "is eating");
		printf("Philosopher %d: last_meal = %ld, meal_eaten = %d\n", philo->id, philo->last_meal, philo->meals_eaten);
		usleep(prog->time_to_eat * 1000);
		philo->meals_eaten++; // 更新吃的饭数
		
		if (philo->meals_eaten >= philo->num_times_to_eat) 
		{
          	pthread_mutex_unlock(&prog->forks[philo->id % prog->num_philos]); 
			pthread_mutex_unlock(&prog->forks[philo->id - 1]); 
            break; // 退出循环
        }
		if (check_death(philo, prog))
		{
   			pthread_mutex_unlock(&prog->forks[philo->id % prog->num_philos]);
    		pthread_mutex_unlock(&prog->forks[philo->id - 1]);
    		break;  // 退出循环
		}

		pthread_mutex_unlock(&prog->forks[philo->id % prog->num_philos]); 
		pthread_mutex_unlock(&prog->forks[philo->id - 1]); 
		print_status(prog, philo, "is sleeping");
		usleep(prog->time_to_sleep * 1000);
        philo->eating = 0; 

	}
	return (NULL);
}

void	creat_thread(t_prog *prog)
{
	int i;
	// pthread_t monitor_thread; 
	i = -1;
	while( ++i < prog->num_philos)
	{
		if (pthread_create(&prog->philos[i].thread, NULL, philosopher_routine, &prog->philos[i]) != 0)
			error_message("Error: pthread_create failed\n");	
	}
	// if (pthread_create(&monitor_thread, NULL, monitor, prog) != 0)
	// 	error_message("Error: pthread_create for monitor failed\n");
	i = -1;
	while( ++i < prog->num_philos)
		pthread_join(prog->philos[i].thread, NULL);
	// pthread_join(monitor_thread, NULL); // 确保监视线程结束
}
	
void	init_philos(t_prog *prog, char **argv)
{
	int	i;

	i = -1;
	while (++i < prog->num_philos)
	{		
		prog->philos[i].id = i + 1;
		if (argv[5])
			prog->philos[i].num_times_to_eat = ft_atoi(argv[5]);
		else
			prog->philos[i].num_times_to_eat = -1;
		prog->philos[i].eating = 0;
		prog->philos[i].meals_eaten = 0;
		prog->philos[i].last_meal = prog->start_time;
		prog->philos[i].dead = &prog->dead_flag;
		prog->philos[i].print_lock = &prog->print_lock;
		prog->philos[i].dead_lock = &prog->dead_lock;
		prog->philos[i].meal_lock = &prog->meal_lock;
		prog->philos[i].prog = prog;
		init_mutex(&prog->forks[i]);
		init_mutex(&prog->forks[(i + 1) % prog->num_philos]);
	}
}

void	init_prog(t_prog *prog, char **argv)
{
	prog->dead_flag = 0;
	prog->time_to_die = ft_atoi(argv[2]);
	prog->time_to_eat = ft_atoi(argv[3]);
	prog->time_to_sleep = ft_atoi(argv[4]);
	init_mutex(&prog->dead_lock);
	init_mutex(&prog->meal_lock);
	init_mutex(&prog->print_lock);
	prog->num_philos = ft_atoi(argv[1]);
	init_philos(prog, argv);
}

void	free_program(t_prog *prog, int num_philos)
{
	int	i;

	i = -1;
	while (++i < num_philos)
		pthread_mutex_destroy(&prog->forks[i]);
	pthread_mutex_destroy(&prog->dead_lock);
	pthread_mutex_destroy(&prog->meal_lock);
	pthread_mutex_destroy(&prog->print_lock);
}

int	main(int argc, char**argv)
{
	t_prog	prog;

	if (argc < 5 || argc > 6)
	{
		printf("Error: wrong number of arguments\n");
		exit(1);
	}
	check_param(argc, argv);
	validation_params(argc, argv);
	init_time(&prog);
	init_prog(&prog, argv);
	creat_thread(&prog);
	free_program(&prog, prog.num_philos);
	return (0);
}
