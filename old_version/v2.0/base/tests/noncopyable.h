#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable 
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable&);
    const noncopyable& operator=(const noncopyable&);
};

#endif