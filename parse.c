#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


enum subsystem_t {
	null = 0,
	tty = 1,
	usb = 2,
};

enum action_t {
	adding = 1,
	removing = 2,
};

struct config {
	enum subsystem_t subsystem;
	enum action_t action;
	char idvendor[512];
	char idproduct[512];
} configs[512];


#define CONFIG_FILE "./rules"


int parse_token(int line, const char *token, int len)
{
	static int counter = 0;
	printf("----: %d / %d : %s\n", line, len, token);
	return 0;
}


int main(int argc, const char *argv[])
{
	int config = open(CONFIG_FILE, O_RDONLY);
	if (config == -1) {
		printf("Error opening config file %s: %s[%d]\n", CONFIG_FILE, strerror(errno), errno);
		return 1;
	}
	
	char config_file[4096];
	memset(config_file, '\0', 4096);
	int r = read(config, config_file, 4096);
	if (r == -1) {
		printf("Error reading from file %s: %s[%d]\n", CONFIG_FILE, strerror(errno), errno);
		return 1;
	}
	
	int i = 0;
	int j = 0;
	int k = 1;
	char config_line[1024];
	memset(config_line, '\0', 1024);
	char tokens[1024];
	char token[1024];
	memset(tokens, '\0', 1024);
	memset(token, '\0', 1024);
	int line = 1;
	int len_line = 0;
	int n = 0;
	int t = 0;
	int delim = 0;
	int c = 0;
	int s = 0;
	
	while (config_file[i] != '\0') {
		memset(config_line, '\0', 1024);
		line = 1;
		while (line) {
			if (config_file[i] == '\n') {
				line = 0;
				break;
			} else {
				config_line[j] = config_file[i];
			}
			i++;
			j++;
		}
		
		printf("LINE: n==%d, len==%d: %s\n", k, j, config_line);
		
		len_line = j;
		printf("====>>>>\n");
		for (n = 0; n < len_line; n++) {
			if ((config_line[n] == ' ') && delim) {
				delim = 0;
				continue;
			}
			if (config_line[n] == ',') {
				delim = 1;
			}
			
			tokens[t] = config_line[n];
			t++;
		}
		
		printf("\tTOKEN LINE:%s\n", tokens);
		
		n = 0; c = 0;
		while (n <= t) {
			if ((tokens[n] == ',') || (n == t)) {
				printf("\t\tTOKEN VALUE:%s[%d]\n", token, c);
				// parse_token(k, token, c);
				if (memcmp("SUBSYSTEM==\"tty\"", token, 16) == 0) {
					printf("MATCH: subsystem TTY\n");
					configs[s].subsystem = tty;
				} else if (memcmp("ACTION=\"add\"", token, 12) == 0) {
					printf("MATCH: action add\n");
					configs[s].action = adding;
				} else if (memcmp("ACTION=\"remove\"", token, 15) == 0) {
					printf("MATCH: action remove\n");
					configs[s].action = removing;
				} else if (memcmp("ATTRS{idVendor}==", token, 17) == 0) {
					printf("MATCH: ATTR(S) idvendor\n");
					if ((17 + 1 <= c) && (token[17] == '"') && (token[c-1] == '"')) {
						printf("VENDOR==%s\n", token+17);
					} else {
						printf("MALFORMED KEY=\"VALUE\"!!!\n");
					}
				} else {
					printf("invalid token: %s!\n", token);
				}
				memset(token, '\0', 1024);
				c = 0;
				if (tokens[n] == ',') {
					n++;
					continue;
				}
			}
			token[c++] = tokens[n++];
		}
		s++;
		printf("\n<<<<====\n");
		i++;
		j = 0;
		k++;
		t = 0;
		memset(tokens, '\0', 1024);
	}
	
	close(config);
	
	return 0;
}


