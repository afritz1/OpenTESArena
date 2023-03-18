#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T>
class Singleton
{
public:
	Singleton() { }
	virtual ~Singleton() { }

	Singleton(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;

	Singleton &operator=(const Singleton&) = delete;
	Singleton &operator=(Singleton&&) = delete;

	static T &getInstance()
	{
		static T instance;
		return instance;
	}
};

#endif
