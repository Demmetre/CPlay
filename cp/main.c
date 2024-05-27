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
#include <dirent.h>
#include <string.h>

extern int is_directory(const char *path);

#ifndef CP_OPTIONS_H
#define CP_OPTIONS_H

struct cp_options
{
    bool recurse;
    bool verbose;
};

#endif

static struct option const long_opts[] = 
{
    {"recurse", no_argument, NULL, 'r'},
    {"verbose", no_argument, NULL, 'v'},
    {"target-directory", required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}   
};

const char *shor_opts = "rvt:";

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

int copy_file(char const *source, char const *target, bool verbose)
{
    if(verbose)
    {
        printf("Copying from %s to %s\n", source, target);
    }
    int src_fd = open(source, O_RDONLY);
    if(src_fd < 0)
    {
        error(EXIT_FAILURE, 0, "Could not open source file %d", source);
        return -1;
    }

    int dst_fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dst_fd < 0)
    {
        error(EXIT_FAILURE, 0, "Could not open destination file %d", target);
        return -1;
    }
    
    int pipe_fds[2];
    if(pipe(pipe_fds) == -1){
        error(EXIT_FAILURE, 0, "Could not create pipe");
        return -1;
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

void main_loop(char const *source, char const *target, struct cp_options *opts)
{
    char current_src[1000];
    char current_dst[1000];
    if(!is_directory(source))
    {
        copy_file(source, target, opts->verbose);
        return;
    }
        
    if (mkdir(target, 0777) == -1)
    {
        error(EXIT_FAILURE, 0, "Could not create directory %d", target);
    }
    
    DIR *d;
    struct dirent *dir;
    d = opendir(source);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] == '.')
            {
                continue;
            }
            
            sprintf(current_src, "%s%s\0", source, dir->d_name);
            sprintf(current_dst, "%s/%s\0", target, dir->d_name);

            if(is_directory(current_src))
            {
                if(opts->recurse)
                {
                    if(!fork())
                    {
                        sprintf(current_src, "%s/\0", current_src);
                        main_loop(current_src, current_dst, opts);
                        break;
                    }
                }
            }
            else
            {
                copy_file(current_src, current_dst, opts->verbose);  
            }
        }
        closedir(d);
    }
}

int main(int argc, char **argv)
{

    char const *source = NULL;
    char const *target = NULL;

    struct cp_options opts;
    opts.recurse = false;
    opts.verbose = false;
    int c;
    while ((c = getopt_long(argc, argv, shor_opts, long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 'r':
            opts.recurse = true;
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

    main_loop(source, target, &opts);
    
    return 0;
}
