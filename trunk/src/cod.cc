/* -*- Mode: C++; c-file-style: "GNU"; comment-column: 40 -*- */
/*
   Copyright (C) 1998,1999 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//
// cod.cc
//
//  The file contains the code for reading Byte Craft's .cod
// formatted symbol files. (Technically this is a c++ file. 
// However, I use the same include file (cod.h) that's used
// by gpasm (which is written in C). Consequently
//
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "../config.h"
#include "gpsim_def.h"
#include "pic-processor.h"
#include "picdis.h"
#include "symbol.h"
#include "cod.h"

#ifdef HAVE_GUI
#include "../gui/gui_interface.h"
void gui_new_program (unsigned int);
#endif

#include "fopen-path.h"

extern int use_gui;

static FILE *codefile = NULL;
static FILE *lstfp = NULL;
//static char *directory_block_data = NULL;
static char *temp_block = NULL;
static char *lstfilename = NULL;

static pic_processor *ccpu = NULL;
DirBlockInfo main_dir;

// Define a flag that tells whether or not we should care about the
// case of text strings in the .cod file. 
static int ignore_case_in_cod = 1;

static int gpasm_recent = 0;

enum cod_errors
{
  COD_SUCCESS,
  COD_FILE_NOT_FOUND,
  COD_UNRECOGNIZED_PROCESSOR,
  COD_FILE_NAME_TOO_LONG,
  COD_LST_NOT_FOUND,
  COD_BAD_FILE
};

char * substr(char *a, char *b, int n)
{
    char *temp;
  *a = 0;
  temp=strncat(a,b,n);
  assert(strlen(a)<=n);
  return temp;
}

// Capitalize a string (there must be a library function that does this!

void strtoupper(char *s)
{
  if(!s)
    return;

  while(*s)
    {
      *s = toupper(*s);
      s++;
    }
}

void strtolower(char *s)
{
  if(!s)
    return;

  char *t = s;

  if(verbose)
    cout << "tolower " << s;
  while(*s)
    {
      *s =  tolower(*s);
      s++;
    }

  if(verbose)
    cout << "after " << t <<'\n';

}

unsigned short get_short_int( char * buff)
{
  return ( (unsigned char)buff[0] + ((unsigned char)buff[1] << 8));
}

void read_block(char * block, int block_number)
{
  fseek(codefile, block_number * COD_BLOCK_SIZE, SEEK_SET);
  fread(block, COD_BLOCK_SIZE, 1, codefile);
}

// reverse the endian type (big to little or little to big)
// note: there's probably a wonderful macro nestled deep in
// the bowels of the os that nicely handles this conversion...

unsigned int endian_swap(unsigned int a)
{

  return
    ( (a>>24) & 0x000000ff) | 
    ( (a>>8)  & 0x0000ff00) |
    ( (a<<8)  & 0x00ff0000) |
    ( (a<<24) & 0xff000000);

}

//-----------------------------------------------------------
// cod_address_in_range - check to see if an address falls into
// one of the valid code areas. A code area is define by a start
// address and an end address. If the address is in a valid area
// then a '1' is returned.
//

int cod_address_in_range(char *range_block,int address)
{
  int i =0;
  int start,end;

  do
    {
      // get the start and end addresses of this range

      start = get_short_int(&range_block[i])/2;
      i+=2;

      end = get_short_int(&range_block[i])/2;
      i+=2;

      if((address>=start) && (address<=end))
	return 1;  // in range

      // The end address can be zero on only the first 
      // start/end pair.

      if((end == 0) && (i>4))
	return 0;

    }
  while(i<COD_BLOCK_SIZE);

  return 0;
}

//-----------------------------------------------------------
// read_hex_from_cod - this routine will get the opcodes from
// the .cod file and intialize the pic program memory with them.
//

void read_hex_from_cod( pic_processor *cpu )
{
  int _64k_base;
  int safety = 0;
  int i,j,index,address;
  char range_block[COD_BLOCK_SIZE];
  DirBlockInfo *dbi;

  dbi = &main_dir;

  do {

    i = get_short_int(&dbi->dir.block[COD_DIR_MEMMAP]);
    j = get_short_int(&dbi->dir.block[COD_DIR_MEMMAP+2]);

    if( (i!=j) || (i==0))
      {
	cout << ".cod range error \n";
	return;
      }

    _64k_base = get_short_int(&dbi->dir.block[COD_DIR_HIGHADDR]) << 15;

    read_block(range_block, i);

    // Loop through all of the .cod file blocks that (may) contain code

    for(i=0; i<COD_CODE_IMAGE_BLOCKS; i++)
      {

	index = get_short_int(&dbi->dir.block[2*(COD_DIR_CODE + i)]);

	if (index != 0) 
	  {
	    read_block(temp_block, index);
	    for(j=0; j<COD_BLOCK_SIZE/2; j++)
	      {
		address = i*COD_BLOCK_SIZE/2 + j;
		if(cod_address_in_range(range_block, address)) {
		  cpu->init_program_memory(address+_64k_base, (int)get_short_int(&temp_block[j*2]));
		}
	      }
	  }
      }

    dbi = dbi->next_dir_block_info;

  } while(dbi && ++safety<10);

}

//-----------------------------------------------------------
FILE *open_a_file(char **filename)
{
  FILE *t;

  if(verbose)
    cout << "Trying to open a file: " << *filename << '\n';

  if(NULL != (t = fopen_path(*filename,"r")))
    return t;

  if(!ignore_case_in_cod)
    return NULL;

  strtoupper(*filename);
  if(NULL != (t = fopen_path(*filename,"r")))
    return t;

  strtolower(*filename);
  if(NULL != (t = fopen_path(*filename,"r")))
    return t;

  cout << "couldn't open " << *filename << " (or any upper/lower case variation)\n";

  return NULL;

}
//-----------------------------------------------------------
// Determing the .lst file name from the cod file file name.
// imo this is cheezy because the .cod file and .lst file have
// to have the same base file name. By convention, mpasm always
// made sure this happened. gpasm otoh, gives you an option to
// make the two different. Furthermore, gpasm includes the .lst 
// file file name in the list of source files within the .cod
// file - unfortunately mpasm doesn't ... so gpsim has to assume
// the list file isn't present

int cod_open_lst(char *filename)
{
  char *pc;
  int i;

  FILE *t;

  lstfilename = strdup(filename);
  pc = strrchr(lstfilename, '.');
  if (pc == NULL) {
    if( (i = strlen(lstfilename)) < (256-4))
      pc = lstfilename + i;
    else
      return COD_FILE_NAME_TOO_LONG;
      
  }
  strcpy(pc, ".lst");

  // Now, let's see if we can open the file
  if(NULL == (t = open_a_file(&lstfilename)))
    return COD_LST_NOT_FOUND;

  fclose(t);

  return COD_SUCCESS;
}



//-----------------------------------------------------------
void read_src_files_from_cod(pic_processor *cpu)
{
#define FILE_SIZE  64
#define FILES_PER_BLOCK (COD_BLOCK_SIZE/FILE_SIZE)

  int i,j,start_block,end_block,offset,num_files;
  char b[FILE_SIZE];

  if(cpu->files) 
    {
      cout << "This cpu already has source files\n";
      return;
    }

  FILE *t;

  num_files = 0;
  end_block = 0;			// eliminates a (spurious) warning
  //start_block = get_short_int(&directory_block_data[COD_DIR_NAMTAB]);
  start_block = get_short_int(&main_dir.dir.block[COD_DIR_NAMTAB]);

  // First, just count the number of source files
  // These may be duplicates, but this is an upper bound
  if(start_block) {
    //    end_block   = get_short_int(&directory_block_data[COD_DIR_NAMTAB+2]);
    end_block   = get_short_int(&main_dir.dir.block[COD_DIR_NAMTAB+2]);
    for(j=start_block; j<=end_block; j++) {
      read_block(temp_block, j);
      for(i=0; i<FILES_PER_BLOCK; i++) {
	offset = i*FILE_SIZE;
	if(temp_block[offset])
	  num_files++;
      }
    }
    if(verbose)
      printf ("Found up to %d source files in .cod file\n", num_files);
  }

  int found_lst_in_cod = 0;

  if(num_files) {
    cpu->files = new file_context[num_files+1+MAX_HLL_FILES];
    cpu->number_of_source_files = num_files;
    cpu->lst_file_id = num_files;

    for(i=0;i<num_files+1+MAX_HLL_FILES;i++)
    	cpu->files[i].file_ptr = NULL;

    num_files = 0;  // now use 'num_files' as a counter.

    for(j=start_block; j<=end_block; j++) {
      read_block(temp_block, j);
      for(i=0; i<FILES_PER_BLOCK; i++) {
	int	alreadyExists = 0;
	int	jj;
	char	*filenm;

	offset = i*FILE_SIZE;
	substr(b,&temp_block[offset+1],FILE_SIZE);
	filenm = b;

	if ((filenm[0] >= 'A') && (filenm[0] <= 'Z')
	    && (':' == filenm[1]) && ('\\' == filenm[2])) {
	  char *cp;
	  filenm += 3;			// strip C:\ from MPLAB files
					// convert \ to / now???
	  for (cp = filenm; *cp; ++cp) { // convert DOS slash to Unix slash
	    if ('\\' == *cp) *cp = '/';
	  }

	}

	for (jj = 0; jj < num_files; ++jj) { // check if already opened
	  // this could be fooled by search paths
	  if (0 == strcasecmp (filenm, cpu->files[jj].name)) {
	    alreadyExists = 1;
	    if (verbose)
	      printf ("Found redundant source file %s\n", filenm);
					// fix:: this leaks memory
	    cpu->number_of_source_files--; // adjust total file count
	    break;
	  }
	}
	if(temp_block[offset] && !alreadyExists) {

	  //cpu->files[num_files].name = new char [strlen(b)];
	  //strcpy(cpu->files[num_files].name,b);

	  cpu->files[num_files].name = strdup(filenm);

	  if(verbose)
	    printf("%s\n",cpu->files[num_files].name);

	  cpu->files[num_files].file_ptr = open_a_file(&cpu->files[num_files].name);
	  
	  cpu->files[num_files].max_line = 0;

	  if((strncmp(lstfilename, filenm,256) == 0) && 
	     (cpu->lst_file_id > cpu->number_of_source_files))
	    {
	      if(verbose)
		printf("Found list file %s\n",cpu->files[num_files].name);
	      cpu->lst_file_id = num_files;
	      found_lst_in_cod = 1;
	    }

	  num_files++;
	}
      }
    }

    if(verbose)
      printf ("Found %d source files in .cod file\n", num_files);

    if(num_files != cpu->number_of_source_files)
      cout << "warning, number of sources changed from " << num_files << " to " << cpu->number_of_source_files << " while reading code (gpsim bug)\n";

    if(!found_lst_in_cod)
      {
	cpu->number_of_source_files = num_files+1; // cpu->lst_file_id;
	cpu->files[num_files].name = strdup(lstfilename);
	//	  cpu->files[cpu->number_of_source_files].file_ptr = NULL;
	if(verbose)
	  printf("List file %s wasn't in .cod\n",cpu->files[num_files].name);
	cpu->files[num_files].file_ptr = 
	  open_a_file(&cpu->files[num_files].name);

	cpu->files[num_files].max_line = 0;
      }

  }else
    printf("No source file info\n");
}

//-----------------------------------------------------------
void read_line_numbers_from_cod(pic_processor *cpu)
{
  int lst_line_number = 0;
  int last_src_line = 0;
  char buf[256];
  int i,j,start_block,end_block,offset;
  int address,file_id, sline,smod;

  //  start_block = get_short_int(&directory_block_data[COD_DIR_LSTTAB]);
  start_block = get_short_int(&main_dir.dir.block[COD_DIR_LSTTAB]);

  if(start_block) {

    //    end_block   = get_short_int(&directory_block_data[COD_DIR_LSTTAB+2]);
    end_block   = get_short_int(&main_dir.dir.block[COD_DIR_LSTTAB+2]);

    // Loop through all of the .cod file blocks that contain line number info

    for(j=start_block; j<=end_block; j++) {

      read_block(temp_block,j);

      // Get the line number info from within one .cod block

      for(offset=0; offset<(COD_BLOCK_SIZE-COD_LS_SIZE); offset += COD_LS_SIZE) {

	if((temp_block[offset+COD_LS_SMOD] & 4) == 0) {
	  lst_line_number++;
	  file_id = temp_block[offset+COD_LS_SFILE];
	  address = get_short_int(&temp_block[offset+COD_LS_SLOC]);
	  sline   = get_short_int(&temp_block[offset+COD_LS_SLINE]);
	  smod    = temp_block[offset+COD_LS_SMOD] & 0xff;

	  if( (file_id <= cpu->number_of_source_files) &&
	      (address <= cpu->program_memory_size()) &&
	      (smod == 0x80) ) {
	    cpu->attach_src_line(address,file_id,sline,lst_line_number);

	  }
	}
      }
    }
    cpu->read_src_files();
  }

}

//-----------------------------------------------------------
// open_cod_file
//
void read_symbols( pic_processor *cpu )
{
  char *s,length;
  short type;
  int i,j,start_block,end_block, value;
  char b[256];

  //start_block = get_short_int(&directory_block_data[COD_DIR_LSYMTAB]);
  start_block = get_short_int(&main_dir.dir.block[COD_DIR_LSYMTAB]);

  if(start_block) {

    //end_block   = get_short_int(&directory_block_data[COD_DIR_LSYMTAB+2]);
    end_block   = get_short_int(&main_dir.dir.block[COD_DIR_LSYMTAB+2]);

    for(j=start_block; j<=end_block; j++) {

      read_block(temp_block, j);

      for(i=0; i<COD_BLOCK_SIZE;) {
	s =  &temp_block[i];

	if(*s==0)
	  break;

	length = *s;
	type  = *(short *)&s[length+1];
	if(type>128)
	  type = COD_ST_CONSTANT;
	value = *(int *)&s[length+3];
	value = endian_swap(value);

	switch(type) 
	  {
	  case COD_ST_C_SHORT:
	    // Change the register name to its symbolic name
	    cpu->registers[value]->new_name(substr(b,&s[1],length));
	    cout << cpu->registers[value]->name() << '\n';
	    symbol_table.add_register(cpu,cpu->registers[value]);
	    break;

	  case COD_ST_ADDRESS:
	    symbol_table.add_address(cpu,substr(b,&s[1],length), value);
	    cout << "symbol at address " << value << " name " <<substr(b,&s[1],length) <<'\n';
	    break;
	    //COD_ST_CONSTANT:
	  default:
	    symbol_table.add_constant(cpu,substr(b,&s[1],length),value);
	  }


	i += (length + 7);
      }
    }
  }else
      printf("No long symbol table info\n");

}

/*---------------------------------------------*/
void clear_block(Block *b)
{

  if(b && b->block)
    bzero(b->block, COD_BLOCK_SIZE);
  else
    assert(0);
}

/*---------------------------------------------*/
void create_block(Block *b)
{

  assert(b != NULL);

  b->block = (char *)malloc(COD_BLOCK_SIZE);
  clear_block(b);
  
}

void delete_block(Block *b)
{

  if(b && b->block) {
    free(b->block);
    b->block = NULL;
  }
  else
    assert(0);

}

/*------------------------------------------------------------------
 * read_directory - read the directory block(s) in the .cod file
 */

void read_directory(void)
{
  DirBlockInfo *dbi;

  create_block(&main_dir.dir);
  read_block(main_dir.dir.block, 0);

  dbi = &main_dir;

  do {
    int next_dir_block = get_short_int(&dbi->dir.block[COD_DIR_NEXTDIR]);

    if(next_dir_block) {
      dbi->next_dir_block_info = (DirBlockInfo *)malloc(sizeof(DirBlockInfo));
      dbi = dbi->next_dir_block_info;
      create_block(&dbi->dir);
      read_block(dbi->dir.block, next_dir_block);
    } else {
      dbi->next_dir_block_info = NULL;
      return;
    }
  } while(1);
}

void delete_directory(void)
{
  //write me.
}

void check_for_gpasm(char *block)
{
  char buffer[256];
  int have_gpasm = 0;

  substr(buffer,&block[COD_DIR_COMPILER],12);

  if(strcmp("gpasm",buffer) == 0) {
    if(verbose)
      cout << "Have gpasm\n";
    have_gpasm = 1;

    substr(buffer,&block[COD_DIR_VERSION],19);

    // Extract gpasm's version numbers
    int major=0, minor=0, micro=0;
    sscanf(&buffer[6],"%d.%d.%d",&major,&minor,&micro);

    if(verbose)
      cout << "gpasm version major "<< major << " minor " << minor << " micro " << micro << endl;

    // if gpasm version is greater than or equal to 0.8.5, then gpasm 
    // is considered "recent"
    if( (major >= 1) || ( minor > 8) || ( (minor == 8) && (micro >= 5)))
      gpasm_recent = 1;

  }



  if(have_gpasm && gpasm_recent) {
    if(verbose)
      cout << "good, you have a recent version of gpasm\n";
  }  else {
    cout << "Warning, you need to upgrade to gpasm-0.8.5 or higher\n";
    cout << "(You're assembler version is  " << buffer << ")\n";
  }
}
//-----------------------------------------------------------
// Read .c line numbers from special .asm files.
void read_hll_line_numbers_from_asm(pic_processor *cpu)
{
    int i;
    struct file_context *gpsim_file;
    char *file_name;
    char filename[256];
    char text_buffer[256];
    int line_number;
    char *srcptrbegin;
    char *ptr;
    FILE *file;
    int line_nr;
    int address;
    int asmfile_id;
    int asmsrc_line;
    int found_line_numbers=0;

    struct {
	char filename[256];
	struct file_context *file;
    }hll_source_files[MAX_HLL_FILES];
    int nr_of_hll_files=0;
    int hll_files_index;
    int file_index;
    int filearray_index;

    // Find the file context that contain the .asm file.
    // This assumes 'there can be only one'.
    for(i=0;i<cpu->number_of_source_files;i++)
    {
	gpsim_file = &cpu->files[i];
	file_name = gpsim_file->name;
	if(!strcmp(file_name+strlen(file_name)-4,".asm"))
	{
	    // Make sure that the file is open
	    if(gpsim_file->file_ptr == NULL)
	    {
		gpsim_file->file_ptr = fopen_path(file_name,"r");
		if(gpsim_file->file_ptr == NULL)
		{
		    printf("file \"%s\" not found!!!\n",file_name);
		    return;
		}
	    }
            break;
	}
    }

    if(i==cpu->number_of_source_files)
    {
        puts("Could not find .asm file!");
	return;
    }

    // Reset hll_file_id and hll_src_line throughout cpu memory
    for(address=0;cpu->program_memory_size()>address;address++)
    {
	cpu->program_memory[address]->hll_file_id=0;
	cpu->program_memory[address]->hll_src_line=0;
    }

    // asmfile_id is index into file context array.
    asmfile_id=i;

    for(i=0;cpu->files[i].file_ptr!=NULL;i++)
    	;
    filearray_index=i;

    rewind(gpsim_file->file_ptr);
    asmsrc_line=0;

    // Loop through the whole .asm file and look for any ";#CSRC" markers
    while(fgets(text_buffer,sizeof(text_buffer),gpsim_file->file_ptr)!=NULL)
    {
        char *ptr2;
	asmsrc_line++;
	if(0!=strncmp(text_buffer,";#CSRC",6))
	    continue;

        // Found a line marker

	ptr=text_buffer+7;
	for(;*ptr!='\0';ptr++)
	    if(*ptr==' '||*ptr=='\t')
		break;
	if(*ptr=='\0')
	    continue; // Syntax error

	// file name
	for(ptr2=text_buffer+7;ptr2<ptr;ptr2++)
            filename[ptr2-(text_buffer+7)]=*ptr2;
	filename[ptr2-(text_buffer+7)]=0;

        ptr++;
	for(;*ptr!='\0';ptr++)
	    if(isdigit(*ptr) || isspace(*ptr))
                break;
	if(!isdigit(*ptr))
	    continue; // Syntax error

	found_line_numbers=1; // The .asm file contains line numbers.

	line_number=atoi(ptr);

	// Locate filename in hll_source_files[]
	for(hll_files_index=0;hll_files_index<nr_of_hll_files;hll_files_index++)
	{
	    if(!strcmp(filename,hll_source_files[hll_files_index].filename))
	    {
		// Found it!
		file_index=hll_files_index+cpu->number_of_source_files;
                break;
	    }
	}
	if(nr_of_hll_files==MAX_HLL_FILES)
	{
            printf("Too many hll files, increase MAX_HLL_FILES\n");
	}
	else if(hll_files_index==nr_of_hll_files)
	{
	    // Add new file

	    int maxline;

	    file_index=hll_files_index+cpu->number_of_source_files;
	    strcpy(hll_source_files[hll_files_index].filename,filename);
            hll_source_files[hll_files_index].file=NULL;
	    nr_of_hll_files++;

	    cpu->files[file_index].name=strdup(filename);
	    file=fopen(cpu->files[file_index].name,"r");
	    if(file==NULL)
	    {
		puts("file is not found\n");
		assert(0);
	    }
	    cpu->files[file_index].file_ptr=file;

	    rewind(cpu->files[file_index].file_ptr);
	    maxline=0;
	    while(fgets(text_buffer,sizeof(text_buffer),cpu->files[file_index].file_ptr)!=NULL)
		maxline++;

	    // Make a new file context
	    cpu->files[file_index].line_seek=NULL;//new int[maxline+1];
	    cpu->files[file_index].max_line=maxline;

	    cpu->files[file_index+1].file_ptr=NULL; // End of list

	    line_nr=0;
//	    cpu->files[file_index].line_seek[line_nr++]=0;

	}


        address=cpu->find_closest_address_to_line(asmfile_id, asmsrc_line);
	if(address >= 0) {
	  cpu->program_memory[address]->hll_src_line=line_number;
	  cpu->program_memory[address]->hll_file_id=file_index;
	}

    }
    if(found_line_numbers)
    {
	cpu->number_of_source_files+=nr_of_hll_files;
    }
    else
    {
	cpu->files[i].file_ptr=NULL;
    }

    // Find first valid line number.
    for(address=cpu->program_memory_size()-1;address>=0;address--)
    {
	if(cpu->program_memory[address]->hll_src_line)
	{
	    line_number=cpu->program_memory[address]->hll_src_line;
	    file_index=cpu->program_memory[address]->hll_file_id;
	}
    }

    // Fill the addresses in the gaps.
    for(address=0;cpu->program_memory_size()>address;address++)
    {
	if(cpu->program_memory[address]->hll_src_line)
	{
	    line_number=cpu->program_memory[address]->hll_src_line;
	    file_index=cpu->program_memory[address]->hll_file_id;
	}
	if(cpu->program_memory[address]->isa()!=instruction::INVALID_INSTRUCTION)
	{
	    cpu->program_memory[address]->hll_file_id=file_index;
	    cpu->program_memory[address]->hll_src_line=line_number;
	}
    }
}

//-----------------------------------------------------------
// open_cod_file
//
//  The purpose of this function is to process a .cod symbol file.
// If a cpu hasn't been declared prior to calling this function, then this
// function will attempt to determine the cpu from the .cod file.
//

int open_cod_file(pic_processor **pcpu, char *filename)
{
  int buffer_size;
  int suspicions = 0; // count the number of legal but suspicious items in the .cod file
  char processor_name[16],*pc;
  int error_code= COD_SUCCESS;
  char directory[256], *dir_path_end;

  cout << "processing cod file " << filename << '\n';

  dir_path_end=strrchr(filename,'/');
  
  if(dir_path_end!=NULL)
  {
      strncpy(directory,filename,dir_path_end-filename);
      directory[dir_path_end-filename]=0;
      printf("directory is \"%s\"\n",directory);
      chdir(directory);
      filename=dir_path_end+1;
      printf("filename is \"%s\"\n",filename);
  }
  
  codefile = fopen(filename,"r");

  if(codefile == NULL) {
    printf("Unable to open %s\n",filename);
    return COD_FILE_NOT_FOUND;
  }

  display_symbol_file_error(cod_open_lst(filename));

  temp_block = new char[COD_BLOCK_SIZE];

  /* Start off by reading the directory block */

  //  directory_block_data = new char[COD_BLOCK_SIZE];

  read_directory();

  // Perform a series of integrity checks

  check_for_gpasm(main_dir.dir.block);

  // If we get here, then the .cod file is good.

  if(*pcpu == NULL) 
    {
      if(verbose)
	cout << "ascertaining cpu from the .cod file\n";
      //substr(processor_name,&directory_block_data[COD_DIR_PROCESSOR],8);
      substr(processor_name,&main_dir.dir.block[COD_DIR_PROCESSOR],8);

      processor_name[8] = 0;

      if(verbose)
	cout << "found a " << processor_name << " in the .cod file\n";

      *pcpu = add_processor(processor_name,processor_name);
      if(*pcpu == NULL) {
	if(!ignore_case_in_cod)
	  return(COD_UNRECOGNIZED_PROCESSOR);

	// Could be that there's a case sensitivity issue:
	strtolower(processor_name);
	*pcpu = add_processor(processor_name,processor_name);

	if(*pcpu == NULL)
	  return(COD_UNRECOGNIZED_PROCESSOR);
      }
    }
  else
    cout << "cpu is non NULL\n";

  ccpu = *pcpu;

  // assume no configuration word is in the cod file.
  ccpu->set_config_word(ccpu->config_word_address(),0xffff);

  read_hex_from_cod(ccpu);

  read_src_files_from_cod(ccpu);

  // Associate the .lst and .asm files' line numbers with 
  // the assembly instructions' addresses.

  read_line_numbers_from_cod(ccpu);
  read_symbols(ccpu);

  // If the .asm file contains special HLL source line comment, then
  // read these and put the HLL line numbers into each instruction.
  read_hll_line_numbers_from_asm(ccpu);

  //delete directory_block_data;
  delete_directory();
  delete temp_block;
  fclose(codefile);

  ccpu->reset(POR_RESET);

#ifdef HAVE_GUI
  if(use_gui){
    gui_new_program(ccpu->processor_id);
    gui_new_source(ccpu->processor_id);
  }
#endif

  return error_code;

}


void display_symbol_file_error(int err)
{

  switch(err) {

    //  case COD_SUCCESS:
    //cout << "no error\n";
    //break;

  case COD_FILE_NOT_FOUND:
    cout << "unable to find the symbol file\n";
    break;

  case COD_UNRECOGNIZED_PROCESSOR:
    cout << "unrecognized processor in the symbol file\n";
    break;

  case COD_BAD_FILE:
    cout << "bad file format\n";
    break;
  }
}
