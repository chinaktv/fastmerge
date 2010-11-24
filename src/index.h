/*
 * =====================================================================================
 *
 *       Filename:  index.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2010年11月23日 16时39分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

void index_init();
int index_add_file(const char *filename);
size_t index_read_line(char *buffer, size_t size);
char *index_read_line_pos(size_t pos, char *buffer, size_t size);
void index_free();
