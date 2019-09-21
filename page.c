//Ken Desrosiers - kdesrosiers
//Jack Hogan - jfhogan
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define memSize 64
#define pageSize 16
#define DISK 5
unsigned char memory[memSize];
//use in roundRobin() to keep track of which page to evict
int pageToEvict = 0;
//indexed by pages, set to zero if free, one if taken up
int freeList[4] = {0, 0, 0, 0};
//keeps track of which pages are page tables, indexed by pages
int isPageTable[4] = {0, 0, 0, 0};
//keeps track of the process of the physical page, indexed by pages
int Proc[4] = {0,0,0,0};
//keeps track of virtual address of each non-page table entry, indexed by page
int processVirtualAddress[4] = {0,0,0,0};
//the simulated disk
FILE *file;
//page table is an array of page tables
typedef struct{
	char valid;
	char pfn;
	char protection;
	char present;
}pageTableEntry;
//indexed by process, stores starting address of page table
int hardwareRegister[4] = {-1, -1, -1, -1};


//round robin function to pick which page to evict next
int roundRobin(){
	if( pageToEvict < 3){
		pageToEvict++;
	}
	else{
		pageToEvict = 0;
	}
	return pageToEvict;
}


//function to find a free page
int findFreePage(){
	int freePage;
	for(int i = 0; i < 4; i++){
		if (freeList[i] == 0){
			freePage = i;
			break;
		}
		else{
			freePage = -1;
		}
	}
	return freePage;
}

//function to send a page to disk to make more room
int sendToDisk(){
	char buf[17];
	int offset;
	int present;
	int vpn;
	//if its a page table
	if(isPageTable[pageToEvict] == 1){
		//offset into the file
		offset = Proc[pageToEvict] * 80;
		for(int i = 0; i < 16; i++){
			//copying 16 bytes into buffer and zeroing memory
			buf[i] = memory[(pageToEvict*16)+i];
			memory[(pageToEvict*16)+i] = 0;
		}
		//go straight the desired offset
		fseek(file, offset, SEEK_SET);
		//write the contents of the buffer to the file
		fwrite(buf,1,16,file);
		//force it to actually write and update
		fflush(file);
		//mark the page table as in disk and update mark the page as free
		hardwareRegister[Proc[pageToEvict]] = DISK;
		freeList[pageToEvict] = 0;
		isPageTable[pageToEvict] = 0;
		printf("Sending page table for process %d to disk!\n",Proc[pageToEvict]);
	}
	//if its a virtual address space
	else{
		//calculating the correct values/locations
		vpn = (processVirtualAddress[pageToEvict] & 0x30) >> 4;
		offset = (Proc[pageToEvict] *80) + (16*(vpn+1));
		present = (hardwareRegister[Proc[pageToEvict]]*16) + (4*vpn) + 3;
		for(int i = 0; i < 16; i++){
			//copying whats in memory to the buffer and zeroing the memory
			buf[i] = memory[(pageToEvict*16)+i];
			memory[(pageToEvict*16)+i] = 0;
		}
		//same as above
		fseek(file, offset, SEEK_SET);
		fwrite(buf,1,16,file);
		fflush(file);
		freeList[pageToEvict] = 0;
		memory[present] = 0;
		printf("Sending process %d's virtual page %d to disk!\n",Proc[pageToEvict],vpn);
	}
	return 0;
}


//function to get a page table form disk
void fetchPageTable(int proc){
	//calculating the offset into the file
	int offset = proc * 80;
	char buf[17];
	int page = findFreePage();
	if(page != -1){
		//if a free page was found
		fseek(file, offset, SEEK_SET);
		fread(buf, 1,16,file);
		freeList[page] = 1;
		for(int i = 0; i < 16; i++){
			memory[(page * 16)+i] = buf[i];
		}
		hardwareRegister[proc] = page;
		isPageTable[page] = 1;
		Proc[page] = proc;
		printf("Loaded page table for process %d from disk into physical frame %d!\n",proc,page);
	}
	//else we have to call round robin, send something to disk, and run fetchPageTable again
	else{
		roundRobin();
		sendToDisk();
		fetchPageTable(proc);
	}
}

//function to get a non page table page from disk
void getFromDisk(int virt, int proc){
	//same sort of process to find a virtual address space in memory
	int offset;
	int vpn;
	int present;
	int pfn;
	char buf[17];
	vpn = (virt & 0x30) >> 4;
	offset = (proc*80) + ((vpn+1)*16);
	present = (hardwareRegister[proc]*16) + (4*vpn) + 3;
	pfn = (hardwareRegister[proc]*16) + (4*vpn) + 1;
	int anotherpage = findFreePage();
	if(anotherpage != -1){
		//if it finds a free page
		fseek(file,offset,SEEK_SET);
		fread(buf,1,16,file);
		freeList[anotherpage] = 1;
		for(int i = 0; i < 16; i++){
			memory[(anotherpage*16)+i] = buf[i];
		}
		memory[present] = 1;
		memory[pfn] = anotherpage;
		Proc[anotherpage] = proc;
		processVirtualAddress[anotherpage] = virt;
		printf("Loaded virtual page %d for process %d from disk into physical frame %d!\n",vpn,proc,anotherpage);
	}
	//else it has to evict a page to make room
	else{
		//checks to make sure it not evicting its own page table
		roundRobin();
		if((Proc[pageToEvict] == proc) && (isPageTable[pageToEvict] == 1)){
			roundRobin();
		}
		sendToDisk();
		getFromDisk(virt,proc);
	}
}

//not updating the page table after i bring a virtual address space back to memory
int createPage(int virt, int procNum, int rw){
	int vpn = (virt & 0x30) >> 4;
	//check to see if a mapping already exists
	//pageTable[vpn].valid
	if(memory[(hardwareRegister[procNum] *16) + (4*vpn)] == 1){
		//first check to see if its in memory!
		if(memory[(hardwareRegister[procNum]*16) + (4*vpn) + 3] == 1){
		//check to see if the rw will even be updated
		//pageTable[vpn].protection
			if(memory[(hardwareRegister[procNum]*16) + (4*vpn) +2] == rw){
				printf("Virtual page %d already has these permissions!\n",vpn);
			}
			else{
				printf("Updated the permissions on virtual page %d!\n",vpn);
				memory[(hardwareRegister[procNum]*16) + (4*vpn) +2] = rw;
			}
		}
		//if it's in disk!
		else{
			//swap and make sure to skip over its page table!
			getFromDisk(virt, procNum);
			createPage(virt, procNum, rw);
		}
	}
	//if a mapping does not exist
	else{
		int secondFreePage = findFreePage();
		if(secondFreePage != -1){
			freeList[secondFreePage] = 1;
			//pageTable[vpn].protection
			memory[(hardwareRegister[procNum]*16) + (4*vpn) +2] = rw;
			//pageTable[vpn].present
			memory[(hardwareRegister[procNum]*16) + (4*vpn) +3] = 1;
			//pageTable[vpn].pfn
			memory[(hardwareRegister[procNum]*16) + (4*vpn) +1] = secondFreePage;
			//pageTable[vpn].valid
			memory[(hardwareRegister[procNum]*16) + (4*vpn)] = 1;
			printf("Mapped virtual address %d (page %d) for process %d into physical frame %d!\n",virt,vpn,procNum,secondFreePage);
			Proc[secondFreePage] = procNum;
			processVirtualAddress[secondFreePage] = virt;
		}
		else{
			//make sure not to swap the page table for procNum
			roundRobin();
			if((Proc[pageToEvict] == procNum) && (isPageTable[pageToEvict] == 1)){
				roundRobin();
			}
			sendToDisk();
			createPage(virt, procNum, rw);
		}
	}
		return 0;	
}

int map(int virt, int procNum, int rw){
	int vpn = (virt & 0x30) >> 4;
	char temp;
	//first check to see if a page table exists for the process

	//if one does not exist
	if(hardwareRegister[procNum] == -1){
		pageTableEntry pageTable[4];
		int freePage = findFreePage();
		//seeing if free page actually exists
		if(freePage != -1){
			freeList[freePage]=1;
			pageTable[vpn].valid = 0;
			pageTable[vpn].pfn = 0;
			pageTable[vpn].protection = 0;
			pageTable[vpn].present = 0;
			for(int i = 0; i < 4; i++){
				for(int j = 0; j < 4; j++){
					if(j == 0){
						temp = pageTable[i].valid;
					}
					else if(j == 1){
						temp = pageTable[i].pfn;
					}
					else if(j == 2){
						temp = pageTable[i].protection;
					}
					else{
						temp = pageTable[i].present;
					}
					memory[(freePage * pageSize) + (i*4) + j] = temp;
				}
			}
			hardwareRegister[procNum] = freePage;
			isPageTable[freePage] = 1;
			Proc[freePage] = procNum;
			processVirtualAddress[freePage] = virt;
			printf("Put a page table for process %d into physical frame %d!\n",procNum,freePage);
		}
		//if there is no free page, swap and make one
		else{
			roundRobin();
			sendToDisk();
			map(virt, procNum, rw);
		}
	}
	//if the page table does exist somewhere (in memory or disk)
	else{
		//if it is in disk
		if (hardwareRegister[procNum] == DISK){
			fetchPageTable(procNum);
			map(virt, procNum, rw);
		}
	}
	createPage(virt,procNum,rw);
	return 0;
}



int load(int virt, int proc, int value){
	int vpn = (virt & 0x30) >> 4;
	int processPFN = memory[((hardwareRegister[proc]*16) + 1 + (4*vpn))];
	int physicalMemoryLocation = (processPFN*pageSize) + (virt%16);
	int present = memory[((hardwareRegister[proc]*16) + 3 + (4*vpn))];
	int valid = memory[((hardwareRegister[proc]*16) + (4*vpn))];
	if(hardwareRegister[proc] != DISK){
		if(valid == 1){
			if(present == 1){
				printf("The value stored in virtual address %d of process %d is %d!\n",virt,proc,memory[physicalMemoryLocation]);
			}
			else{
				getFromDisk(virt,proc);
				load(virt,proc,value);
			}
		}
		else{
			printf("Virtual page %d for process %d has not been allocated yet!\n",vpn,proc);
		}
	}
	else{
		fetchPageTable(proc);
		load(virt,proc,value);
	}
	return 0;
}

int store(int virt, int proc, int val){
	int vpn = (virt & 0x30) >> 4;
	int processPFN = memory[(hardwareRegister[proc]*16) + 1 + (4*vpn)];
	int physicalMemoryLocation = (processPFN*pageSize) + (virt%16);
	int present = memory[((hardwareRegister[proc]*16) + 3 + (4*vpn))];
	int valid = memory[((hardwareRegister[proc]*16) + (4*vpn))];
	int writeable = memory[((hardwareRegister[proc]*16) + 2 +  (4*vpn))];
	if(hardwareRegister[proc] != DISK){
		if(valid == 1){
			if(writeable == 1){
				if(present == 1){
					memory[physicalMemoryLocation] = val;
					printf("Storing the value of %d into virtual address %d of process %d!\n",val,virt,proc);
				}
				else{
					getFromDisk(virt,proc);
					store(virt,proc,val);
				}
			}
			else{
				printf("This virtual address isn't writeable!\n");
			}
		}
		else{
			printf("Virtual page %d for process %d has not been allocated yet!\n",vpn,proc);
		}
	}
	else{
		fetchPageTable(proc);
		store(virt,proc,val);
	}
	return 0;
}



int main(){
	for(int i = 0; i < 63; i++){
		memory[i] = 0;
	}
	file = fopen("disk.txt", "r+");
		char input[50];
		int process_id, virtual_address, value;
		char instruction_type[20];
		int arrayIndex = 0;
		char *arrayofargs[4];
	while(1){
		printf("Instruction? ");
		scanf("%s",input);
		arrayofargs[arrayIndex] = strtok(input, ",");
		while(arrayofargs[arrayIndex] != NULL){
			arrayIndex++;
			arrayofargs[arrayIndex] = strtok(NULL, ",");
		}
		//put the inputs into seperate variables
		process_id = atoi(arrayofargs[0]);
		strcpy(instruction_type, arrayofargs[1]);
		virtual_address = atoi(arrayofargs[2]);
		value = atoi(arrayofargs[3]);
		//handle which instruction to run
		if(strcmp(instruction_type, "map") == 0){
			map(virtual_address, process_id, value);
		}
		else if(strcmp(instruction_type, "load") == 0){
			load(virtual_address, process_id, value);
		}	
		else if(strcmp(instruction_type, "store") == 0){
			store(virtual_address, process_id, value);
		}
		else{
			printf("Invalid Instruction!\n");
		}
		memset(input, 0, sizeof(input));
		arrayIndex = 0;
	}
	return 0;
}