#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <cassert>
#include <cstddef>

#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;
#include "base.h"

/*
Простой планировщик заданий а-ля крон
-- программа, запускающая другие программы, согласно некоторому расписанию
список заданий для запуска -- в текстовом файле
имя программы (+ аргументы), частота запуска

после запуска заданий он отслеживает их статус
  * успешно ли запустилась
  * работает ли до сих пор
  * не запускаем следующий раз, если ещё выполняется

управляющая консоль, запросы к планировщику
  * список выполняющих заданий
  * запрос на перечтение текстового файла
*/

vector<Program> ps;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void reloadInfoFile()
{
	pthread_mutex_lock(&mutex);
	map<string, Program> all;
	for (Program &p : ps)
		all[p.name] = p;
	ps.clear();
	
	fstream fin("info.txt");
	Program p;
	while (fin >> p.name >> p.milliseconds)
	{
		auto it = all.find(p.name);
		if (it != all.end())
		{
			it->second.milliseconds = p.milliseconds;
			p = it->second;
			all.erase(it);
		}
		ps.push_back(p);
	}
	pthread_mutex_unlock(&mutex);
	fin.close();
}

void updateProgramsStatus()
{
	pthread_mutex_lock(&mutex);
	for (Program &p : ps)
		if (p.pid != 0 && p.running)
		{
			//cout << "before " << p << endl;
			int rc;
			check(waitpid(p.pid, &rc, WNOHANG));
			check(rc);
			p.running = !WIFEXITED(rc);
			p.exitStatus = WEXITSTATUS(rc);
			//cout << "after  " << p << endl;
		}
	pthread_mutex_unlock(&mutex);
}

long getmtime()
{
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

void runProgram(Program &p)
{
	cout << "run " << p << endl;
	if (p.running)
		return;
	pid_t pid = fork();
	check(pid);
	if (pid == 0)
	{
		const char *name = p.name.c_str();
		int rc = execlp(name, name, 0);
		check(rc);
	}
	else
	{
		p.pid = pid;
		p.lastRunTime = getmtime();
		p.running = true;
	}
}

void tryRun()
{
	updateProgramsStatus();
	long curr_mtime = getmtime();
	for (Program &p : ps)
	{
		long dmtime = curr_mtime - p.lastRunTime;
		assert(dmtime >= 0);
		//dbg(curr_mtime, dmtime, p);
		if (dmtime > p.milliseconds)
			runProgram(p);
	}
}

void *thread_start(void *arg)
{
	// socket()
	int server_sockfd;
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		perror("socket"), exit(1);
	
	// bind
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	
	struct hostent *h;
	if ((h = gethostbyname("localhost")) == NULL)
		herror("gethostbyname"), exit(1);
	memcpy(&server_addr.sin_addr, h->h_addr, h->h_length);
	
	if (bind(server_sockfd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == -1)
		perror("bind"), exit(1);
	
	// listen
	if (listen(server_sockfd, 0) == -1)
		perror("listen"), exit(1);
	
	// accept
	printf("Listen for requests...\n");
	while (1)
	{
		int client_sockfd;
		struct sockaddr_in client_addr;
		socklen_t client_addrlen = 0;
		if ((client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_addrlen)) == -1)
			perror("accept"), exit(1);
		
		char request_buf[1024];
		int len;
		check(len = read(client_sockfd, request_buf, sizeof(request_buf)));
		if (len >= 1024)
		{
			printf("Unknown request\n");
			close(client_sockfd);
			continue;
		}
		request_buf[len] = 0;
		string request = string(request_buf);
		dbg(request);
		if (request == "getinfo")
		{
			vector<uint8_t> data;
			pthread_mutex_lock(&mutex);
			data.push_back(ps.size());
			for (Program &p : ps)
				p.write(data);
			pthread_mutex_unlock(&mutex);
			
			int size = data.size();
			uint8_t *buffer = (uint8_t *) malloc(size);
			for (int i = 0; i < size; ++i)
				buffer[i] = data[i];
			check(write(client_sockfd, buffer, size));
		}
		else if (request == "reload")
			reloadInfoFile();
		close(client_sockfd);
		printf("request %s done\n", request.c_str());
	}
	return 0;
}

int main()
{
	pthread_t id; //unused
	pthread_create(&id, NULL, &thread_start, 0);
	
	reloadInfoFile();
	while (1)
	//for (int i = 0; i < 5; ++i)
	{
		tryRun();
		long curr_mtime = getmtime();
		long minTime = 1e15;
		for (Program &p : ps)
			minTime = min(minTime, p.getNextRunTime(curr_mtime));
		long diff = minTime - curr_mtime;
		assert(diff >= 0);
		usleep(diff * 1000 + 100);
	}
	return 0;
}
