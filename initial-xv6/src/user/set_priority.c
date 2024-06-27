
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
// #include "string.h"



int main(int argc,char* argv[])
{
    // struct proc* p;
    char* ch1=argv[1];
    char* ch2=argv[2];
    int pid=atoi(ch1);
    int new_prior=atoi(ch2);
    if(new_prior>100||new_prior<0)
    {
        printf("Invalid Priority number\n");
        return 0;
    }
    int call=set_priority(pid,new_prior);
    if(call<0)
    {
        printf("PID doesn't exist");
    }
    return 0;
}

