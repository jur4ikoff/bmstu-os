#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <limits.h>

#define BUF_SIZE 4096

void print_file(const char *path, FILE *out)
{
    char buf[BUF_SIZE];
    size_t n;
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(out, "error open %s\n", path);
        return;
    }
    while ((n = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
        buf[n] = '\0';
        fputs(buf, out);
    }
    fclose(f);
}

void print_symlink(const char *path, FILE *out)
{
    char target[PATH_MAX];
    ssize_t len = readlink(path, target, sizeof(target) - 1);
    if (len == -1) {
        fprintf(out, "error open %s\n", path);
        return;
    }
    target[len] = '\0';
    fprintf(out, "%s\n", target);
}

void print_environ(const char *pid, FILE *out)
{
    char path[PATH_MAX];
    char buf[BUF_SIZE];
    size_t n;
    size_t i;
    FILE *f;

    fprintf(out, "\nENVIRON (/proc/%s/environ)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/environ", pid);
    f = fopen(path, "r");
    if (!f) {
        fprintf(out, "error open\n");
        return;
    }
    while ((n = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
        for (i = 0; i < n; i++)
            if (buf[i] == '\0') buf[i] = '\n';
        buf[n] = '\0';
        fputs(buf, out);
    }
    fclose(f);
}

const char *state_description(char state)
{
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk sleep";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        case 't': return "Tracing stop";
        case 'W': return "Waking / Paging";
        case 'X': return "Dead";
        case 'K': return "Wakekill";
        case 'P': return "Parked";
        default:  return "неизвестное состояние";
    }
}

void print_stat(const char *pid, FILE *out)
{
    char path[PATH_MAX];
    char buf[BUF_SIZE];
    char token[512];
    char *p;
    char *end;
    int idx;
    int nfields;
    size_t len;

    const char *field_names[] = {
        "pid", "comm", "state", "ppid", "pgrp",
        "session", "tty_nr", "tpgid", "flags", "minflt",
        "cminflt", "majflt", "cmajflt", "utime", "stime",
        "cutime", "cstime", "priority", "nice", "num_threads",
        "itrealvalue", "starttime", "vsize", "rss", "rsslim",
        "startcode", "endcode", "startstack", "kstkesp", "kstkeip",
        "signal", "blocked", "sigignore", "sigcatch", "wchan",
        "nswap", "cnswap", "exit_signal", "processor", "rt_priority",
        "policy", "delayacct_blkio_ticks", "guest_time", "cguest_time",
        "start_data", "end_data", "start_brk", "arg_start", "arg_end",
        "env_start", "env_end", "exit_code",
    };

    fprintf(out, "\nSTAT (/proc/%s/stat)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(out, "error open\n");
        return;
    }
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return;
    }
    fclose(f);

    nfields = (int)(sizeof(field_names) / sizeof(field_names[0]));
    p = buf;
    idx = 0;
    while (*p && idx < nfields) {
        if (idx == 1 && *p == '(') {
            end = strrchr(p, ')');
            if (end) {
                len = (size_t)(end - p) + 1;
                if (len >= sizeof(token)) len = sizeof(token) - 1;
                strncpy(token, p, len);
                token[len] = '\0';
                p = end + 2;
            } else {
                strncpy(token, p, sizeof(token) - 1);
                token[sizeof(token) - 1] = '\0';
                p += strlen(token);
            }
        } else {
            len = strcspn(p, " \n");
            if (len >= sizeof(token)) len = sizeof(token) - 1;
            strncpy(token, p, len);
            token[len] = '\0';
            p += len;
            if (*p == ' ' || *p == '\n') p++;
        }
        fprintf(out, "  [%2d] %-24s %s\n", idx + 1, field_names[idx], token);
        if (idx == 2 && token[0])
            fprintf(out, "       %s\n", state_description(token[0]));
        idx++;
    }
}

void print_cmdline(const char *pid, FILE *out)
{
    char path[PATH_MAX];
    int c;
    FILE *f;

    fprintf(out, "\nCMDLINE (/proc/%s/cmdline)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/cmdline", pid);
    f = fopen(path, "r");
    if (!f) {
        fprintf(out, "error open\n");
        return;
    }
    while ((c = fgetc(f)) != EOF)
        fputc(c == '\0' ? ' ' : c, out);
    fputc('\n', out);
    fclose(f);
}

void print_fd(const char *pid, FILE *out)
{
    char dirpath[PATH_MAX];
    char linkpath[PATH_MAX + NAME_MAX + 2];
    char target[PATH_MAX];
    struct dirent *de;
    ssize_t len;
    DIR *dp;

    fprintf(out, "\nFD (/proc/%s/fd/)\n", pid);

    snprintf(dirpath, sizeof(dirpath), "/proc/%s/fd", pid);
    dp = opendir(dirpath);
    if (!dp) {
        fprintf(out, "error open dir\n");
        return;
    }
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        snprintf(linkpath, sizeof(linkpath), "%s/%s", dirpath, de->d_name);
        len = readlink(linkpath, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            fprintf(out, "  fd %-4s -> %s\n", de->d_name, target);
        }
    }
    closedir(dp);
}

void print_maps(const char *pid, FILE *out)
{
    char path[PATH_MAX];
    char line[512];
    char addr_field[64];
    char perms[8];
    char rest[512];
    char *dash;
    uint64_t start, end, pages;
    long page_size;
    FILE *f;

    fprintf(out, "\nMAPS (/proc/%s/maps)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/maps", pid);
    f = fopen(path, "r");
    if (!f) {
        fprintf(out, "error open\n");
        return;
    }
    uint64_t total_pages = 0;
    page_size = sysconf(_SC_PAGE_SIZE);
    while (fgets(line, sizeof(line), f)) {
        rest[0] = '\0';
        if (sscanf(line, "%63s %7s%[^\n]", addr_field, perms, rest) < 2) continue;
        dash = strchr(addr_field, '-');
        if (!dash) { fputs(line, out); continue; }
        *dash = '\0';
        start = strtoull(addr_field, NULL, 16);
        end   = strtoull(dash + 1, NULL, 16);
        pages = (end - start) / (uint64_t)page_size;
        total_pages += pages;
        {
            char *r = rest;
            while (*r == ' ') r++;
            fprintf(out, "%s-%s %-6lu %s %s\n",
                addr_field, dash + 1, (unsigned long)pages, perms, r);
        }
    }
    fprintf(out, "\nИтого страниц: %lu, таблиц страниц: %lu\n",
        (unsigned long)total_pages,
        (unsigned long)((total_pages + 511) / 512));
    fclose(f);

    /* точный vsize из smaps */
    snprintf(path, sizeof(path), "/proc/%s/smaps", pid);
    f = fopen(path, "r");
    if (f) {
        uint64_t smaps_kb = 0;
        unsigned long kb;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Size: %lu kB", &kb) == 1)
                smaps_kb += kb;
        }
        fclose(f);
        fprintf(out, "vsize по smaps:  %lu байт (%lu страниц)\n",
            (unsigned long)(smaps_kb * 1024),
            (unsigned long)(smaps_kb / 4));
    }
}

void print_pagemap(const char *pid, FILE *out)
{
    char path[PATH_MAX];
    char line[512];
    char addr_field[64];
    char *dash;
    uint64_t start, end, addr, index, entry;
    uint64_t total, present, swapped, file_shared;
    long page_size;
    int pm_fd;
    FILE *maps;

    fprintf(out, "\nPAGEMAP (/proc/%s/pagemap)\n", pid);
    fprintf(out, "  %-28s %8s %8s %8s %8s\n",
        "region", "total", "present", "swapped", "file");

    snprintf(path, sizeof(path), "/proc/%s/maps", pid);
    maps = fopen(path, "r");
    if (!maps) {
        fprintf(out, "error open maps\n");
        return;
    }
    snprintf(path, sizeof(path), "/proc/%s/pagemap", pid);
    pm_fd = open(path, O_RDONLY);
    if (pm_fd == -1) {
        fprintf(out, "error open\n");
        fclose(maps);
        return;
    }
    page_size = sysconf(_SC_PAGE_SIZE);
    while (fgets(line, sizeof(line), maps)) {
        if (sscanf(line, "%63s", addr_field) != 1) continue;
        dash = strchr(addr_field, '-');
        if (!dash) continue;
        *dash = '\0';
        start = strtoull(addr_field, NULL, 16);
        end   = strtoull(dash + 1, NULL, 16);
        total = 0; present = 0; swapped = 0; file_shared = 0;
        for (addr = start; addr < end; addr += (uint64_t)page_size) {
            index = (addr / (uint64_t)page_size) * sizeof(uint64_t);
            entry = 0;
            if (pread(pm_fd, &entry, sizeof(entry), (off_t)index) != (ssize_t)sizeof(entry))
                continue;
            total++;
            if ((entry >> 63) & 1) present++;
            if ((entry >> 62) & 1) swapped++;
            if ((entry >> 61) & 1) file_shared++;
        }
        fprintf(out, "  %s-%s %8lu %8lu %8lu %8lu\n",
            addr_field, dash + 1,
            (unsigned long)total,
            (unsigned long)present,
            (unsigned long)swapped,
            (unsigned long)file_shared);
    }
    fclose(maps);
    close(pm_fd);
}

void print_io(const char *pid, FILE *out)
{
    char path[PATH_MAX];

    fprintf(out, "\nIO (/proc/%s/io)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/io", pid);
    print_file(path, out);
}

void print_comm(const char *pid, FILE *out)
{
    char path[PATH_MAX];

    fprintf(out, "\nCOMM (/proc/%s/comm)\n", pid);

    snprintf(path, sizeof(path), "/proc/%s/comm", pid);
    print_file(path, out);
}

void print_task(const char *pid, FILE *out)
{
    char dirpath[PATH_MAX];
    char comm_path[PATH_MAX];
    char comm[64];
    struct dirent *de;
    DIR *dp;
    FILE *cf;

    fprintf(out, "\nTASK (/proc/%s/task/)\n", pid);

    snprintf(dirpath, sizeof(dirpath), "/proc/%s/task", pid);
    dp = opendir(dirpath);
    if (!dp) {
        fprintf(out, "error open dir\n");
        return;
    }
    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        fprintf(out, "  TID %s", de->d_name);
        snprintf(comm_path, sizeof(comm_path), "/proc/%s/task/%s/comm", pid, de->d_name);
        cf = fopen(comm_path, "r");
        if (cf) {
            memset(comm, 0, sizeof(comm));
            if (fgets(comm, sizeof(comm), cf)) {
                comm[strcspn(comm, "\n")] = '\0';
                fprintf(out, "  comm=%s", comm);
            }
            fclose(cf);
        }
        fprintf(out, "\n");
    }
    closedir(dp);
}

void dump_proc_info(const char *pid, const char *label, const char *outfile)
{
    char path[PATH_MAX];
    FILE *out = fopen(outfile, "w");
    if (!out) {
        perror("fopen");
        return;
    }
    fprintf(out, "PID %s (%s)\n", pid, label);

    fprintf(out, "\nCWD (/proc/%s/cwd)\n", pid);
    fprintf(out, "Текущая рабочая директория процесса\n\n");
    snprintf(path, sizeof(path), "/proc/%s/cwd", pid);
    print_symlink(path, out);

    fprintf(out, "\nEXE (/proc/%s/exe)\n", pid);
    fprintf(out, "Исполняемый файл процесса\n\n");
    snprintf(path, sizeof(path), "/proc/%s/exe", pid);
    print_symlink(path, out);

    fprintf(out, "\nROOT (/proc/%s/root)\n", pid);
    fprintf(out, "Корень файловой системы процесса (устанавливается chroot).\n\n");
    snprintf(path, sizeof(path), "/proc/%s/root", pid);
    print_symlink(path, out);

    print_cmdline(pid, out);
    print_environ(pid, out);
    print_stat(pid, out);
    print_fd(pid, out);
    print_maps(pid, out);
    print_pagemap(pid, out);
    print_io(pid, out);
    print_comm(pid, out);
    print_task(pid, out);

    fclose(out);
    printf("PID %s (%s) -> %s\n", pid, label, outfile);
}

int main(int argc, char *argv[])
{
    char outfile[64];
    if (argc < 2) {
        fprintf(stderr, "usage: %s <pid> [label outfile]\n", argv[0]);
        return 1;
    }
    if (argc == 4) {
        dump_proc_info(argv[1], argv[2], argv[3]);
    } else {
        snprintf(outfile, sizeof(outfile), "proc_%s.txt", argv[1]);
        dump_proc_info(argv[1], "процесс", outfile);
    }
    return 0;
}
