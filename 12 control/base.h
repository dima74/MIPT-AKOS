#include <string>
#include <vector>
#include <deque>
#include <iostream>
using namespace std;

#define PORT 7783

#define check(expr)			\
{							\
	if ((expr) == -1)		\
	{						\
		perror(#expr);		\
		exit(1);			\
	}						\
}

template<typename T>
ostream& operator <<(ostream& out, vector<T> v)
{
#ifdef LOCAL
	out << '{';
	for (size_t i = 0; i < v.size(); ++i)
		out << (i == 0 ? "" : ", ") << v[i];
	return out << '}';
#endif
}

#define dbg(...) myPrint(#__VA_ARGS__, __VA_ARGS__)
template <typename Arg1>
void myPrint(const char* name, Arg1&& arg1)
{
#ifdef LOCAL
	cerr << name << "=" << arg1 << endl;
#endif
}

template <typename Arg1, typename... Args>
void myPrint(const char* names, Arg1&& arg1, Args&&... args)
{
#ifdef LOCAL
	const char* comma = strchr(names + 1, ',');
	cerr.write(names, comma - names) << "=" << arg1;
	myPrint(comma, args...);
#endif
}

struct Program
{
	string name;
	int milliseconds;
	int pid = 0;
	long lastRunTime = 0;
	bool running = 0;
	int exitStatus;
	
	long getNextRunTime(long curr_mtime)
	{
		return (running ? curr_mtime : lastRunTime) + milliseconds;
	}
	
	void write(vector<uint8_t> &data)
	{
		data.push_back(name.length());
		for (char c : name)
			data.push_back(c);
		data.push_back(running);
	}
	
	void read(deque<uint8_t> &data)
	{
		int len = data.front(); data.pop_front();
		for (int i = 0; i < len; ++i)
			name.push_back(data.front()), data.pop_front();
		running = data.front(); data.pop_front();
	}
};

ostream& operator<<(ostream& out, Program p)
{
	return out << "{" << "name=" << p.name << ", pid=" << p.pid << ", running=" << p.running << ", milliseconds=" << p.milliseconds << "}";
}
