/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mehdimirzaie <mehdimirzaie@student.42.f    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/11 11:07:14 by mmirzaie          #+#    #+#             */
/*   Updated: 2023/10/09 17:38:09 by mehdimirzai      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execute.h"
#include "shell.h"
#include <stdio.h>
#include <sys/wait.h>

static bool	is_last_cmd(int num_cmds, int pipe1[2])
{
	if (num_cmds == 1)
		return (true);
	redirect(pipe1[1], STDOUT_FILENO);
	return (false);
}

static void	open_and_redirect(char *name, int rw, int permission)
{
	int	file_fd;

	if (!permission)
		file_fd = open(name, rw);
	else
		file_fd = open(name, rw, permission);
	if (file_fd < 0)
		error_exit(" No such file or directory\n", EXIT_FAILURE);
	if (!permission)
	{
		if (redirect(file_fd, STDIN_FILENO) < 0)
			error_exit(" No such file or directory\n", EXIT_FAILURE);
	}
	else
		if (redirect(file_fd, STDOUT_FILENO) < 0)
			error_exit(" No such file or directory\n", EXIT_FAILURE);
}

static void	open_file(t_ast *ast, int pipe1[2], int num_cmds)
{
	t_iolst	*redirects;

	redirects = ast->cmd->redirects;
	if (redirects == NULL)
		if (is_last_cmd(num_cmds, pipe1))
			return ;
	while (redirects)
	{
		if (redirects->type == E_TTLLA)
		{
			while (redirects->next)
				if (redirects->next->type == E_TTLLA)
					redirects = redirects->next;
			handle_heredoc(ast);
		}
		if (redirects->type == E_TTLA)
			open_and_redirect(redirects->str, O_RDONLY, 0);
		if (redirects->type == E_TTRA || redirects->type == E_TTRRA)
		{
			if (redirects->type == E_TTRA)
				open_and_redirect(redirects->str,
					O_WRONLY | O_CREAT | O_TRUNC, 0644);
			else
				open_and_redirect(redirects->str,
					O_WRONLY | O_CREAT | O_APPEND, 0644);
		}
		redirects = redirects->next;
	}
}

static void	execute(t_ast *ast, t_env **our_env, int *exit_status, int num_cmds)
{
	t_ast	*node;
	int		pipe1[2];

	while (num_cmds > 0)
	{
		node = get_next_node(ast, num_cmds);
		if (pipe(pipe1) < 0)
			perror("error making pipe\n");
		if (fork() == 0)
		{
			signal(SIGINT, SIG_DFL);
			open_file(node, pipe1, num_cmds);
			close(pipe1[0]);
			if (is_builtin(node->cmd) || is_envbuiltin(node->cmd))
				execute_builtin_cmds(node->cmd, our_env, exit_status);
			else
				execute_system_cmds(node->cmd, *our_env);
			exit(EXIT_SUCCESS);
		}
		redirect(pipe1[0], STDIN_FILENO);
		close(pipe1[1]);
		num_cmds--;
	}
}

void	process_ast(t_ast *ast, t_env **our_env, int *exit_status)
{
	const int	in = dup(STDIN_FILENO);
	t_cmd		*first_cmd = NULL;
	t_ast		*first_node = NULL;
	int			num_cmds;

	// if (ast->type == E_ASTCMD)
	// 	first_cmd = ast->cmd;
	// else
	// 	first_cmd = ast->link.first->cmd;
	signal(SIGINT, SIG_IGN);
	first_node = get_next_node(ast, 1);
	first_cmd = first_node->cmd;
	if (!ft_strncmp(first_cmd->cmd, "exit", 5) || is_envbuiltin(first_cmd))
	{
		open_file(ast, NULL, 1);
		execute_builtin_cmds(ast->cmd, our_env, exit_status);
		ast = ast->link.second;
	}
	num_cmds = get_num_cmd(ast);
	execute(ast, our_env, exit_status, num_cmds);
	while (num_cmds-- > 0)
		wait(exit_status);
	wait(NULL);
	dup2(in, STDIN_FILENO);
}
