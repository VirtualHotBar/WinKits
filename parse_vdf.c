#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

struct VdfNode {
    char *key;
    char *value;
    struct VdfNode *child;
    struct VdfNode *next;
};

void skip_whitespace(const char *content, int *pos) {
    while (content[*pos] && isspace(content[*pos])) {
        (*pos)++;
    }
}

char *parse_string(const char *content, int *pos) {
    if (content[*pos] != '"') return NULL;
    (*pos)++;
  
    char *buffer = malloc(strlen(content) + 1);
    int i = 0;
    bool escape = false;

    while (content[*pos]) {
        if (escape) {
            buffer[i++] = content[(*pos)++];
            escape = false;
        } else if (content[*pos] == '\\') {
            escape = true;
            (*pos)++;
        } else if (content[*pos] == '"') {
            (*pos)++;
            break;
        } else {
            buffer[i++] = content[(*pos)++];
        }
    }

    buffer[i] = '\0';
    return realloc(buffer, i + 1);
}

struct VdfNode *parse_node(const char *content, int *pos);

struct VdfNode *parse_vdf(const char *content, int *pos) {
    struct VdfNode *head = NULL;
    struct VdfNode *tail = NULL;

    while (1) {
        skip_whitespace(content, pos);
        if (content[*pos] == '}' || content[*pos] == '\0') break;
      
        struct VdfNode *node = parse_node(content, pos);
        if (!node) break;

        if (!head) head = node;
        else tail->next = node;
        tail = node;
    }

    return head;
}

struct VdfNode *parse_node(const char *content, int *pos) {
    skip_whitespace(content, pos);
    if (content[*pos] != '"') return NULL;

    struct VdfNode *node = malloc(sizeof(struct VdfNode));
    node->key = parse_string(content, pos);
    node->value = NULL;
    node->child = NULL;
    node->next = NULL;

    skip_whitespace(content, pos);

    if (content[*pos] == '"') {
        node->value = parse_string(content, pos);
    } else if (content[*pos] == '{') {
        (*pos)++;
        node->child = parse_vdf(content, pos);
        skip_whitespace(content, pos);
        if (content[*pos] != '}') {
            free(node->key);
            free(node);
            return NULL;
        }
        (*pos)++;
    }

    return node;
}

void free_vdf(struct VdfNode *node) {
    while (node) {
        struct VdfNode *next = node->next;
        free_vdf(node->child);
        free(node->key);
        free(node->value);
        free(node);
        node = next;
    }
}

void list_keys(struct VdfNode *node) {
    while (node) {
        printf("%s\n", node->key);
        node = node->next;
    }
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <vdf-file> <key-path> [list]\n", argv[0]);
        return 1;
    }
    
    bool list_mode = (argc == 4 && strcmp(argv[3], "list") == 0);

    // 读取文件内容
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  
    char *content = malloc(len + 1);
    fread(content, 1, len, fp);
    content[len] = '\0';
    fclose(fp);

    // 解析VDF
    int pos = 0;
    struct VdfNode *root = parse_vdf(content, &pos);
    if (!root) {
        fprintf(stderr, "Invalid VDF format\n");
        free(content);
        return 1;
    }

    // 分割路径
    char *path = strdup(argv[2]);
    char *token = strtok(path, "/");
    struct VdfNode *current_block = root;
    struct VdfNode *target = NULL;

    while (token) {
        struct VdfNode *n = current_block;
        while (n && strcmp(n->key, token) != 0) n = n->next;
      
        if (!n) {
            fprintf(stderr, "Key '%s' not found\n", token);
            free(content);
            free(path);
            free_vdf(root);
            return 1;
        }

        target = n;
        current_block = n->child;
        token = strtok(NULL, "/");
    }

    if (list_mode) {
        if (target && target->child) {
            list_keys(target->child);
        } else {
            fprintf(stderr, "No children found at specified path\n");
        }
    } else {
        if (target && target->value) {
            printf("%s\n", target->value);
        } else {
            fprintf(stderr, "No value found at specified path\n");
        }
    }

    // 清理资源
    free(content);
    free(path);
    free_vdf(root);

    return 0;
}
