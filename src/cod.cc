/* -*- Mode: C++; c-file-style: "GNU"; comment-column: 40 -*- */
/*
   Copyright (C) 1998,1999 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

//
// cod.cc
//
//  The file contains the code for reading Byte Craft's .cod
// formatted symbol files.
//
#include <stdio.h>
#include <iostream>
#include <string>
#include <string.h>
#include <assert.h>
#include <memory.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "../config.h"

#include "exports.h"
#include "gpsim_def.h"
#include "sim_context.h"
#include "pic-processor.h"
#include "picdis.h"
#include "symbol.h"
#include "cod.h"
#include "interface.h"
#include "fopen-path.h"
#include "breakpoints.h"

/*  experiment with assertions */
#include "cmd_manager.h"
/*  end of assertion experiment */

PicCodProgramFileType::PicCodProgramFileType() {
  codefile = 0;
  temp_block = 0;
  lstfilename = 0;

  memset(&main_dir, 0, sizeof(main_dir));

  // Define a flag that tells whether or not we should care about the
  // case of text strings in the .cod file.
  ignore_case_in_cod = 1;

  gputils_recent = 0;
  RegisterProgramFileType(this);
}

int PicCodProgramFileType::get_string(char *dest, char *src, size_t len)
{
  size_t n = *src++;

  if(n < len) {
    n = min(n, len - 1);
    strncpy(dest, src, n);
    dest[n] = '\0';
    return SUCCESS;
  }
  return ERR_BAD_FILE;
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

void PicCodProgramFileType::read_block(char * block, int block_number)
{
  fseek(codefile, block_number * COD_BLOCK_SIZE, SEEK_SET);
  fread(block, COD_BLOCK_SIZE, 1, codefile);
}

unsigned int get_be_int( char * buff)
{
  return ( (unsigned char)buff[3]       + ((unsigned char)buff[2] << 8) +
           ((unsigned char)buff[1] << 16) + ((unsigned char)buff[0] << 24));
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

void PicCodProgramFileType::read_hex_from_cod( Processor *cpu )
{
  int _64k_base;
  int safety = 0;
  int i,j,index;
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

        if (index != 0) {
          read_block(temp_block, index);
          for(j=0; j<COD_BLOCK_SIZE/2; j++) {
            int PCindex  = i*COD_BLOCK_SIZE/2 + j;
            if(cod_address_in_range(range_block, PCindex)) {
              cpu->init_program_memory_at_index(PCindex+_64k_base, (int)get_short_int(&temp_block[j*2]));
            }
          }
        }
      }

    dbi = dbi->next_dir_block_info;

  } while(dbi && ++safety<10);

}

//-----------------------------------------------------------
FILE *PicCodProgramFileType::open_a_file(char **filename)
{
  FILE *t;

  if(verbose)
    cout << "Trying to open a file: " << *filename << '\n';

  if(0 != (t = fopen_path(*filename,"r")))
    return t;

  if(!ignore_case_in_cod)
    return 0;

  strtoupper(*filename);
  if(0 != (t = fopen_path(*filename,"r")))
    return t;

  strtolower(*filename);
  if(0 != (t = fopen_path(*filename,"r")))
    return t;

//  cout << "couldn't open " << *filename << " (or any upper/lower case variation)\n";

  return 0;

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

int PicCodProgramFileType::cod_open_lst(const char *filename)
{
  char *pc;
  int i;

  FILE *t;

  lstfilename = strdup(filename);
  pc = strrchr(lstfilename, '.');
  if (pc == 0) {
    if( (i = strlen(lstfilename)) < (256-4))
      pc = lstfilename + i;
    else
      return ERR_FILE_NAME_TOO_LONG;

  }
  strcpy(pc, ".lst");

  // Now, let's see if we can open the file
  if(0 == (t = open_a_file(&lstfilename)))
    return ERR_LST_FILE_NOT_FOUND;

  fclose(t);

  return SUCCESS;
}



//-----------------------------------------------------------
int PicCodProgramFileType::read_src_files_from_cod(Processor *cpu)
{
#define FILE_SIZE  64
#define FILES_PER_BLOCK (COD_BLOCK_SIZE/FILE_SIZE)
  int iReturn = SUCCESS;
  int i,j,start_block,end_block,offset,num_files;
  char b[FILE_SIZE];

  num_files = 0;
  end_block = 0;                        // eliminates a (spurious) warning
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

    cpu->files.list_id(num_files);
    num_files = 0;  // now use 'num_files' as a counter.

    for(j=start_block; j<=end_block; j++) {
      read_block(temp_block, j);
      for(i=0; i<FILES_PER_BLOCK; i++) {

        char    *filenm;

        offset = i*FILE_SIZE;
        if((iReturn = get_string(b,&temp_block[offset],sizeof b)) != SUCCESS) {
          goto _Cleanup;
        }
        filenm = b;

#ifdef _WIN32
        // convert to DOS style file name
        char *cp;

        // convert Unix slash to DOS slash
        for (cp = filenm; *cp; ++cp) { // convert Unix slash to DOS slash
          if ('/' == *cp)
            *cp = '\\';
        }
#else
        // convert to Unix style file name
        if ((filenm[0] >= 'A') && (filenm[0] <= 'Z')
            && (':' == filenm[1]) && ('\\' == filenm[2])) {
          char *cp;
          filenm += 3;                  // strip C:\ from MPLAB files
                            // convert \ to / now???
          for (cp = filenm; *cp; ++cp) { // convert DOS slash to Unix slash
            if ('\\' == *cp) *cp = '/';
          }
        }
#endif

        string s1 = string(filenm);

        if(temp_block[offset] && (cpu->files.Find(s1) < 0)) {

          //
          // Add this file to the list
          //
          if(cpu->files.Add(filenm)<0) {
            // File not available.
            continue;
          }

          if((strncmp(lstfilename, filenm,256) == 0) &&
              (cpu->files.list_id() >= cpu->files.nsrc_files()) ) {
            if(verbose)
              cout << "Found list file " << ((cpu->files)[num_files])->name() << endl;
            cpu->files.list_id(num_files);
            found_lst_in_cod = 1;
          }

          num_files++;
        }
      }
    }

    if(verbose)
      cout << "Found " << num_files << " source files in .cod file\n";

    if(num_files != cpu->files.nsrc_files())
      cout << "warning, number of sources changed from " << num_files << " to "
           << cpu->files.nsrc_files() << " while reading code (gpsim bug)\n";

    if(!found_lst_in_cod) {
      cpu->files.Add(lstfilename);
      cpu->files.list_id(num_files);

      if(verbose)
        printf("List file %s wasn't in .cod\n",lstfilename);
    }

  } else
    printf("No source file info\n");

_Cleanup:
  if(0){
    // Debug code
    int i;
    cout << " new file stuff: " << cpu->files.nsrc_files() << " new files\n";
    for(i=0; i<cpu->files.nsrc_files(); i++) {
      cout << ((cpu->files)[i])->name() << endl;
    }
  }
  return iReturn;
}

//-----------------------------------------------------------
void PicCodProgramFileType::read_line_numbers_from_cod(Processor *cpu)
{
  int j,start_block,end_block,offset;
  int file_id, sline,smod;
  unsigned int address;

  start_block = get_short_int(&main_dir.dir.block[COD_DIR_LSTTAB]);

  if(start_block) {

    end_block   = get_short_int(&main_dir.dir.block[COD_DIR_LSTTAB+2]);

    // Loop through all of the .cod file blocks that contain line number info

    for(j=start_block; j<=end_block; j++) {

      read_block(temp_block,j);

      // Get the line number info from within one .cod block

      for(offset=0; offset<(COD_BLOCK_SIZE-COD_LS_SIZE); offset += COD_LS_SIZE) {

        if((temp_block[offset+COD_LS_SMOD] & 4) == 0) {
          file_id = temp_block[offset+COD_LS_SFILE];
          address = get_short_int(&temp_block[offset+COD_LS_SLOC]);
          //address = cpu->map_pm_address2index(address);
          sline   = get_short_int(&temp_block[offset+COD_LS_SLINE]);
          smod    = temp_block[offset+COD_LS_SMOD] & 0xff;

          if( (file_id <= cpu->files.nsrc_files()) &&
//              (address <= cpu->program_memory_size()) &&
              cpu->IsAddressInRange(address) &&
              (smod == 0x80) )

            cpu->attach_src_line(address,file_id,sline,0);

        }
      }
    }
    cpu->read_src_files();
  }

}

//-----------------------------------------------------------
// read_message_area(Processor *cpu)
//
// The .cod file message area contains information like assertions
// and simulation scripts.

void PicCodProgramFileType::read_message_area(Processor *cpu)
{
#define MAX_STRING_LEN  255 /* Maximum length of a debug message */
  char DebugType,DebugMessage[MAX_STRING_LEN];

  unsigned short i, j, start_block, end_block;
  unsigned short laddress;

  // If the .cod file contains a simulation script, then we'll
  // pass it to the command line interface. Note, we go through
  // this indirect way of accessing the CLI since we don't wish
  // for code in the src/ directory to depend directly on code
  // in the cli/ (or any other) directory.
  start_block = get_short_int(&main_dir.dir.block[COD_DIR_MESSTAB]);

  if(start_block) {

    end_block = get_short_int(&main_dir.dir.block[COD_DIR_MESSTAB+2]);

    for(i=start_block; i<=end_block; i++) {
      read_block(temp_block, i);

#if 0
      {
        // Debug code to display the contents of the message area.
        int q,p;
        printf ("Codefile block 0x%x\n",i);

        for (q=0,p=0; q < COD_BLOCK_SIZE; q+=16) {

          for (p=0; p<16; p++)
            printf("%02X ",(unsigned char)temp_block[q+p]);
          for (p=0; p<16; p++)
            printf("%c", isascii(temp_block[q+p]) ? temp_block[q+p] : '.');
          printf("\n");
        }
#endif

      j = 0;

      // Each message has the form of
      // AAAAAAAACCstring
      // AAAAAAAA - 32bit address in PIC program memory
      // CC - 8-bit command
      // string - a 0-terminated string of characters.

      while (j < COD_BLOCK_SIZE-8) {

        /* read big endian */
        laddress = get_be_int(&temp_block[j]);

        j += 4;   // 4 = size of big endian

        DebugType = temp_block[j++];

        if (DebugType == 0) {
          break;
        }

        get_string(DebugMessage, &temp_block[j], sizeof DebugMessage);

        j += strlen(DebugMessage)+1;

        if(verbose)
          printf("debug message: addr=%#x command=\"%c\" string=\"%s\"\n",
                  laddress,
                  DebugType,
                  DebugMessage);

        // The lower case commands are user commands.  The upper case are
        // compiler or assembler generated.  This code makes no distinction
        // between them.

        switch(DebugType) {
        // The 'A' and 'E' options in gpasm specifies a list of gpsim commands
        // that are to be executed after the .cod file has been loaded.
        case 'a':
        case 'A':
          // assertion
          {
            string script("directive");
            char buff[256];
            snprintf(buff,sizeof(buff),"break e %d, %s\n",laddress,DebugMessage);
            string cmd(buff);
            cpu->add_command(script,cmd);
          }
          break;
        case 'e':
        case 'E':
          // gpsim command
          {
            string script("directive");
            string cmd(DebugMessage);
            cmd = cmd + '\n';
            cpu->add_command(script,cmd);
          }
          break;

        case 'c':
        case 'C':
          // gpsim command
          // The 'c'/'C' option in gpasm specifies a single gpsim command that is
          // to be invoked whenever the address associated with this directive
          // is being simulated.
          {
            bool bPost = DebugType == 'c';
            CommandAssertion *pCA = new CommandAssertion(cpu,laddress,0,
                                                        DebugMessage,bPost);
            get_bp().set_breakpoint(pCA,cpu);
          }
        case 'f':
        case 'F':
          // printf

          break;
        case 'l':
        case 'L':
          // log

          break;
        default:
          cout << "Warning: unknown debug message \"" << DebugType << "\"\n";
        }
      }
    }
  }

}

//-----------------------------------------------------------
// open_cod_file
//
void PicCodProgramFileType::read_symbols( Processor *cpu )
{
  int iReturn = SUCCESS;
  char *s,length;
  short type;
  int i,j,start_block,end_block, value;
  char b[256];

  start_block = get_short_int(&main_dir.dir.block[COD_DIR_LSYMTAB]);

  if(start_block) {

    end_block   = get_short_int(&main_dir.dir.block[COD_DIR_LSYMTAB+2]);

    for(j=start_block; j<=end_block; j++) {

      read_block(temp_block, j);

      for(i=0; i<COD_BLOCK_SIZE;) {
        s =  &temp_block[i];

        if(*s==0)
            break;

        length = *s;
        type  = get_short_int(&s[length+1]);
        if(type>128)
            type = COD_ST_CONSTANT;
        value = get_be_int(&s[length+3]);

        switch(type) {
        case COD_ST_C_SHORT: {
          // Change the register name to its symbolic name
          iReturn = get_string(b, s, sizeof b);
          cpu->registers[value]->new_name(b);
          //register_symbol *rs = new register_symbol((char*)0, cpu->registers[value]);
          //symbol_table.add(rs);
          //cpu->addSymbol(cpu->registers[value]);
          }
          break;

        case COD_ST_ADDRESS: {
          iReturn = get_string(b, s, sizeof b);
          instruction *pI = cpu->pma->getFromAddress(value);
          if (pI) {
            string s(b);
            pI->addLabel(s);
          }
          //cpu->addSymbol(new AddressSymbol(cpu, b, value));
          //symbol_table.add_address(cpu,b, value);
          }
          break;

        case COD_ST_CONSTANT:   // Ignore as no useful purpose and may
                                // conflict with other symbols - RRR
          break;

        default:
          iReturn = get_string(b,s,sizeof b);
          cpu->addSymbol(new Integer(b,value));
          break;
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
    memset(b->block, 0, COD_BLOCK_SIZE);
  else
    assert(0);
}

/*---------------------------------------------*/
void create_block(Block *b)
{

  assert(b != 0);

  b->block = (char *)malloc(COD_BLOCK_SIZE);
  clear_block(b);

}

void delete_block(Block *b)
{

  if(b && b->block) {
    free(b->block);
    b->block = 0;
  }
  else
    assert(0);

}

/*------------------------------------------------------------------
 * read_directory - read the directory block(s) in the .cod file
 */

void PicCodProgramFileType::read_directory(void)
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
      dbi->next_dir_block_info = 0;
      return;
    }
  } while(1);
}

void PicCodProgramFileType::delete_directory(void)
{
  DirBlockInfo *dbi;
  DirBlockInfo *next;

  next = main_dir.next_dir_block_info;

  while(next != 0) {
      dbi = next;
      next = dbi->next_dir_block_info;
      delete_block(&dbi->dir);
      free(dbi);
  }
  delete_block(&main_dir.dir);
}

int PicCodProgramFileType::check_for_gputils(char *block)
{
  int iReturn = SUCCESS;
  char buffer[256];
  int have_gputils = 0;

  if((iReturn = get_string(buffer,&block[COD_DIR_COMPILER - 1],12)) != SUCCESS) {
    goto _Cleanup;
  }

  if ((strcmp("gpasm",buffer) == 0) || (strcmp("gplink",buffer) == 0)) {
    if(verbose)
      cout << "Have gputils\n";
    have_gputils = 1;

    if((iReturn = get_string(buffer,&block[COD_DIR_VERSION - 1],19)) != SUCCESS) {
      goto _Cleanup;
    }

    int major=0, minor=0, micro=0;
    if (isdigit(buffer[0])) {
      // Extract version numbers in new gputils format
      sscanf(&buffer[0],"%d.%d.%d",&major,&minor,&micro);

      if(verbose)
        cout << "gputils version major "<< major << " minor " << minor << " micro " << micro << endl;

      // if gputils version is greater than or equal to 0.13.0, then gputils
      // is considered "recent"
      if ((major >= 1) || ( minor >= 13))
        gputils_recent = 1;

    } else {
      // version number in old gputils format, so it can't be recent
      gputils_recent = 0;
    }

  }

  if(have_gputils && gputils_recent) {
    if(verbose)
      cout << "good, you have a recent version of gputils\n";
  }  else {
    cout << "Warning, you need to upgrade to gputils-0.13.0 or higher\n";
    cout << "(Your assembler version is  " << buffer << ")\n";
  }
_Cleanup:
  return iReturn;
}

//-----------------------------------------------------------
// Read .c line numbers from special .asm files.
void PicCodProgramFileType::read_hll_line_numbers_from_asm(Processor *cpu)
{
  int line_number;
  unsigned int address;

  int file_index;

	// Reset hll_file_id and hll_src_line throughout cpu memory
	for(address=0;address<cpu->program_memory_size();address++)
	{
		cpu->program_memory[address]->set_hll_file_id(-1);
		cpu->program_memory[address]->set_hll_src_line(0); // Meaning 'not set' in this function.
	}

	// For each file
	int nfiles=cpu->files.nsrc_files();
	for(file_index=0;file_index<nfiles;file_index++) {
		char text_buffer[256];
		// If asm file
		if(!cpu->files[file_index]->name().compare(cpu->files[file_index]->name().length()-4,4,".asm")) {
			int current_hll_file_id;
			int asmsrc_line;
	
			// Loop through the whole .asm file and look for any line markers
			cpu->files[file_index]->rewind();
			asmsrc_line=0;
			while(cpu->files[file_index]->gets(text_buffer,sizeof(text_buffer))!=0)
			{
				asmsrc_line++;
				string fn;
				if(0==strncmp(text_buffer,";\t.line\t",8)) // \t.line\t12345\t"filename"
				{
					char *lineendpos=strchr(text_buffer, ';');
					if(lineendpos==0)
						continue;
					*lineendpos=0;
					line_number=atoi(text_buffer+8);
					char *fnstart = strchr(lineendpos+1, '\"');
					if(fnstart==0)
						continue;
					fnstart++;
					char *fnend = strchr(fnstart+1, '\"');
					if(fnend==0)
						continue;
					*fnend=0;
					fn=fnstart;
				}
				else if(0==strncmp(text_buffer,";#CSRC ",7)) // ;#CSRC filename 12345
				{
					char *fnend = strrchr(text_buffer, ' ');
					if(fnend==0)
						continue;
					*fnend=0;
					fn=text_buffer+7;
					char *linestart=fnend+1;
					line_number=atoi(linestart);
				}
				else
				{
					continue;
				}

				// Add hll file if not already added
				current_hll_file_id = cpu->files.Find(fn);
				if(current_hll_file_id<0)
				{
					current_hll_file_id=cpu->files.Add(fn, true);
					if(current_hll_file_id<0)
						continue;
					cpu->files[current_hll_file_id]->ReadSource();
				}

				// Find closest address of asm line and set hll line number, hll file id and file context pm address.
				address=cpu->pma->find_closest_address_to_line(file_index, asmsrc_line);
				if(address >= 0) {
					cpu->program_memory[address]->set_hll_src_line(line_number);
					cpu->program_memory[address]->set_hll_file_id(current_hll_file_id);
					cpu->files[current_hll_file_id]->put_address(line_number, address);
				}

			}

			// Find address of last asm line and set hll_line -1, so we know the end when filling the gaps.
			address=cpu->pma->find_closest_address_to_line(file_index, asmsrc_line-1);
			if(address>=0)
			{
				cpu->program_memory[address]->set_hll_src_line(-1);
			}
			else
			{
				cout<<"Error did not find address of last asm line!"<<endl;
			}
		}
	}

	// Fill the addresses in the gaps.
	file_index=0;
	line_number=-1;
	for(address=0;address<cpu->program_memory_size();address++)
	{
		int line = cpu->program_memory[address]->get_hll_src_line();
		if(line==0)
		{
			if(cpu->program_memory[address]->isa()!=instruction::INVALID_INSTRUCTION)
			{
				cpu->program_memory[address]->set_hll_file_id(file_index);
				cpu->program_memory[address]->set_hll_src_line(line_number);
			}
		}
		else
		{
			line_number=line;
			file_index=cpu->program_memory[address]->get_hll_file_id();
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
/*
int open_cod_file(Processor **pcpu, const char *filename)
{
  char directory[256];
  const char *dir_path_end;
  dir_path_end = get_dir_delim(filename);

  if(dir_path_end!=0)
  {
      strncpy(directory,filename,dir_path_end-filename);
      directory[dir_path_end-filename]=0;
      printf("directory is \"%s\"\n",directory);
      chdir(directory);
      filename=dir_path_end+1;
      printf("filename is \"%s\"\n",filename);
  }
  return load_cod_file(pcpu, filename, fopen(filename,"rb"));
}
*/

int PicCodProgramFileType::LoadProgramFile(Processor **pcpu,
                                           const char *filename,
                                           FILE *pFile,
                                           const char *pProcessorName)
{
  int error_code= SUCCESS;
  Processor *ccpu = 0;

  codefile = pFile;
  if(codefile == 0) {
    printf("Unable to open %s\n",filename);
    return ERR_FILE_NOT_FOUND;
  }
  error_code= cod_open_lst(filename);
  if(error_code != SUCCESS) {
    display_symbol_file_error(error_code);
    return error_code;
  }

  temp_block = new char[COD_BLOCK_SIZE];

  /* Start off by reading the directory block */

  read_directory();

  // Perform a series of integrity checks

  if((error_code = check_for_gputils(main_dir.dir.block)) != SUCCESS) {
    goto _Cleanup;
  }

  // If we get here, then the .cod file is good.
  if(*pcpu == 0) {

    char processor_type[16];
    processor_type[0] = 'p';  // Hack to get around processors whose name begin with a digit.

    if(verbose)
      cout << "ascertaining cpu from the .cod file\n";

    if(SUCCESS == get_string(&processor_type[1],
                             &main_dir.dir.block[COD_DIR_PROCESSOR - 1],
                             sizeof (processor_type)-1)) {

      char *pProcessorTypeOffset = isdigit(processor_type[1]) ?
        &processor_type[0] : &processor_type[1];

      if (!pProcessorName)
        pProcessorName = pProcessorTypeOffset;

      if(verbose)
        cout << "found a " << processor_type << " in the .cod file\n";

      *pcpu = (Processor *)CSimulationContext::GetContext()->add_processor(processor_type,
                                                                           pProcessorName);
      if(*pcpu == 0) {
        if(!ignore_case_in_cod)
          return(ERR_UNRECOGNIZED_PROCESSOR);

        // Could be that there's a case sensitivity issue:
        strtolower(processor_type);
        *pcpu = (Processor *)CSimulationContext::GetContext()->
          add_processor(processor_type,pProcessorName);

        if(*pcpu == 0)
          return(ERR_UNRECOGNIZED_PROCESSOR);
      }
    }
    else {
      return(ERR_UNRECOGNIZED_PROCESSOR);
    }
  }
  else
    cout << "cpu is non NULL\n";

  ccpu = *pcpu;

  read_hex_from_cod(ccpu);

  ccpu->files.SetSourcePath(filename);
  read_src_files_from_cod(ccpu);

  // Associate the .lst and .asm files' line numbers with
  // the assembly instructions' addresses.

  read_line_numbers_from_cod(ccpu);
  read_symbols(ccpu);

  // If the .asm file contains special HLL source line comment, then
  // read these and put the HLL line numbers into each instruction.
  read_hll_line_numbers_from_asm(ccpu);

  // Read all the debug messages
  read_message_area(ccpu);

_Cleanup:
  //delete directory_block_data;
  delete_directory();
  delete [] temp_block;

  if(*pcpu != NULL) {
    (*pcpu)->reset(POR_RESET);
    bp.clear_global();
    string script("directive");
    (*pcpu)->run_script(script);
  }
  return error_code;

}




void PicCodProgramFileType::display_symbol_file_error(int err)
{

  switch(err) {

  case ERR_FILE_NOT_FOUND:
    cout << "unable to find the symbol file\n";
    break;

  case ERR_UNRECOGNIZED_PROCESSOR:
    cout << "unrecognized processor in the symbol file\n";
    break;

  case ERR_BAD_FILE:
    cout << "bad file format\n";
    break;
  }
}
