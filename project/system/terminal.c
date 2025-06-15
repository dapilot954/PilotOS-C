#include "../drivers/print.h"
#include "../drivers/keyboard.h"
#include "../drivers/port_io.h"
#include "../drivers/fat32.h"
#include <stdint.h>
typedef uint32_t size_t;



// Returns 1 if first n chars of str1 equal to str2, else 0
int starts_with_n(const char* str1, const char* str2, int n) {
    for (int i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return 0; // mismatch
        }
        if (str2[i] == '\0') {
            return 0;
        }
    }
    return 1; // first n chars match
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    // If we haven't copied n chars yet, pad with '\0'
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}


char* strcpy(char* dest, const char* src) {
    char* orig = dest;
    while ((*dest++ = *src++));
    return orig;
}

char last_char(const char* str) {
    const char* p = str;
    while (*p) {
        p++;
    }
    // If string is empty
    if (p == str) {
        return '\0';  // or handle empty-string case as needed
    }
    return *(p - 1);
}



// Copies s1 and s2 into buffer if enough space, returns 1 on success, 0 on failure
int str_concat_into(char* buffer, int buffer_size, const char* s1, const char* s2) {
    int len1 = 0, len2 = 0;

    while (s1[len1] != '\0') len1++;
    while (s2[len2] != '\0') len2++;

    if (len1 + len2 + 1 > buffer_size) {
        return 0;
    }

    for (int i = 0; i < len1; i++) {
        buffer[i] = s1[i];
    }
    for (int i = 0; i < len2; i++) {
        buffer[len1 + i] = s2[i];
    }
    buffer[len1 + len2] = '\0';

    return 1;
}



char* trim_front(char* str, int n) {
    if (n < 0) n = 0;  
    while (n > 0 && *str) {
        str++;
        n--;
    }
    return str;
}



static char path[512] = "/";
void cd_command(const char* arg) {
    char new_path[512];

    if (strcmp(arg, "..") == 0) {
        // Going up a directory
        int len = strlen(path);
        if (len > 1) {
            if (path[len - 1] == '/') len--;  // Remove trailing slash
            while (len > 0 && path[len - 1] != '/') len--;  // Go up
            strncpy(new_path, path, len);
            new_path[len] = '\0';

            // Ensure trailing slash unless root
            if (len > 1 && new_path[len - 1] != '/') {
                new_path[len] = '/';
                new_path[len + 1] = '\0';
            }
        } else {
            strcpy(new_path, "/");
        }
    } else {
        // Build new path with trailing slash
        if (strcmp(path, "/") == 0) {
            str_concat_into(new_path, sizeof(new_path), "/", arg);
        } else {
            char temp[512];
            str_concat_into(temp, sizeof(temp), path, arg);
            str_concat_into(new_path, sizeof(new_path), temp, "");
        }

        // Add trailing slash if not present
        int len = strlen(new_path);
        if (len > 0 && new_path[len - 1] != '/') {
            new_path[len] = '/';
            new_path[len + 1] = '\0';
        }
    }

    // Check if the directory exists
    if (fat32_dir_exists(new_path)) {
        strcpy(path, new_path);
    } else {
        print("Directory does not exist.\n");
    }
}




void terminal_run()
{
    char text[512];
    char* prefix[512];
    str_concat_into(prefix, 512, path, "> ");
    input(prefix, text, 512);

    if (starts_with_n(text, "echo ", 5))
    {
        char* res[512];
        str_concat_into(res, 512, trim_front(text, 5), "\n");
        print(res);
        print("\n");
    }
    else if(starts_with_n(text, "ls", 2))
    {
        fat32_list_directory(path);
        print("\n");
    }
    else if(starts_with_n(text, "shutdown", 8))
    {
        outw(0x604, 0x2000);
    }
    else if (starts_with_n(text, "cd ", 3)) {
        char* arg = trim_front(text, 3);
        cd_command(arg);
        print("\n");
        
    }
    else if (starts_with_n(text, "mkdir ", 6)) {
        char* arg = trim_front(text, 6);
        char mkdir_path[512];
        str_concat_into(mkdir_path, 512, path, arg);
        if (fat32_create_dir(mkdir_path) == true)
        {
            char res[512];
            str_concat_into(res, 512, "created directory ", mkdir_path);
            print(res);
            print("\n");
        }
        else
        {
            print("invalid syntax or directory exists");
            print("\n");
        }
        print("\n");
        
        
    }
    else if (starts_with_n(text, "rmdir  ", 6)) {
        char* arg = trim_front(text, 6);
        char rmdir_path[512];
        str_concat_into(rmdir_path, 512, path, arg);
        if (fat32_delete_dir(rmdir_path) == true)
        {
            char res[512];
            str_concat_into(res, 512, "created directory ", rmdir_path);
            print(res);
            print("\n");
        }
        else
        {
            print("invalid syntax or directory doesnt exists");
            print("\n");
        }
        print("\n");
        
        
    }
    else
    {
        print("invalid syntax or command not found!\n");
        print("\n");
    }
}