#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#ifndef CP_OPTIONS_H
#define CP_OPTIONS_H

struct cp_options
{
    bool verbose;
};

#endif

static struct option const long_opts[] = 
{
    {"verbose", no_argument, NULL, 'v'},
    {"target-directory", required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}   
};

const char *shor_opts = "vt:";

#define BUFFER_SIZE 1024

typedef struct
{
    int src_fd;
    int dst_fd;
    int pipe_fd;
}thread_arg;

void *reader_handler(void *arg)
{
    const thread_arg *params = (const thread_arg *)arg;
    
    int bytes_read;
    char buffer[BUFFER_SIZE];

    while((bytes_read = read(params->src_fd, buffer, BUFFER_SIZE)) > 0)
    {
        write(params->pipe_fd, buffer, bytes_read);
    }

    close(params->pipe_fd);
    pthread_exit(NULL);
}

void *writer_handler(void *arg)
{
    const thread_arg *params = (const thread_arg *)arg;
    
    int bytes_read;
    char buffer[BUFFER_SIZE];

    while((bytes_read = read(params->pipe_fd, buffer, BUFFER_SIZE)) > 0)
    {
        write(params->dst_fd, buffer, bytes_read);
    }

    close(params->pipe_fd);
    pthread_exit(NULL);

}


int main(int argc, char **argv)
{

    char const *source = NULL;
    char const *target = NULL;

    struct cp_options opts = {false};
    int c;
    while ((c = getopt_long(argc, argv, shor_opts, long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 'v':
            opts.verbose = true;
            break;
        case 't':
            target = optarg;
            break;
        default:
            break;
        }

    }
    
    if (optind < argc) 
    {
        source = argv[optind];
        optind++;
    }
    if (optind < argc && !target) 
    {
        target = argv[optind];
    }

    if(!source) 
    {
        error(EXIT_FAILURE, 0, "No source specified");
        return 0;
    }

    if(!target) 
    {
        error(EXIT_FAILURE, 0, "No target specified");
        return 0;
    }

    int src_fd = open(source, O_RDONLY);
    if(src_fd < 0)
    {
        error(EXIT_FAILURE, 0, "Could not open source file %d", source);
        return 0;
    }

    int dst_fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dst_fd < 0)
    {
        error(EXIT_FAILURE, 0, "Could not open destination file %d", target);
        return 0;
    }
    
    int pipe_fds[2];
    if(pipe(pipe_fds) == -1){
        error(EXIT_FAILURE, 0, "Could not create pipe");
        return 0;
    }

    pthread_t reader_t, writer_t;
    thread_arg reader_arg = {src_fd, dst_fd, pipe_fds[1]};
    thread_arg writer_arg = {src_fd, dst_fd, pipe_fds[0]};

    pthread_create(&reader_t, NULL, reader_handler, &reader_arg);
    pthread_create(&writer_t, NULL, writer_handler, &writer_arg);

    pthread_join(reader_t, NULL);
    pthread_join(writer_t, NULL);

    close(src_fd);
    close(dst_fd);

    return 0;
}