#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

struct process_t;

struct system_t{
    
    int process_count;
    struct process_t *process; 
}
#endif