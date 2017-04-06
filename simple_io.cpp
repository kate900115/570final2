
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "cpucounters.h"
#include "utils.h"

using namespace std;

//file read write
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#define EVENT_SIZE 256
#define FootPrint 6000
void build_event(const char* argv, EventSelectRegister *reg, int idx);


struct CoreEvent
{
	char name[256];
	uint64 value;
	uint64 msr_value;
	char* description;
} events[4];

extern "C" {
	SystemCounterState SysBeforeState, SysAfterState;
	std::vector<CoreCounterState> BeforeState, AfterState;
	std::vector<SocketCounterState> DummySocketStates;
	EventSelectRegister regs[4];
	PCM::ExtendedCustomCoreEventDescription conf;

	int pcm_c_build_core_event(uint8_t idx, const char * argv)
	{
		if(idx > 3)
			return -1;

		cout << "building core event " << argv << " " << idx << endl;
		build_event(argv, &regs[idx], idx);
		return 0;
	}

	int pcm_c_init()
	{
		PCM * m = PCM::getInstance();
		conf.fixedCfg = NULL; // default
		conf.nGPCounters = 4;
		conf.gpCounterCfg = regs;
		conf.OffcoreResponseMsrValue[0] = events[0].msr_value;
		conf.OffcoreResponseMsrValue[1] = events[1].msr_value;

		cerr << "\n Resetting PMU configuration" << endl;
		m->resetPMU();
		PCM::ErrorCode status = m->program(PCM::EXT_CUSTOM_CORE_EVENTS, &conf);
		if(status == PCM::Success)
			return 0;
		else
			return -1;
	}

	void pcm_c_start()
	{
		PCM * m = PCM::getInstance();
		m->getAllCounterStates(SysBeforeState, DummySocketStates, BeforeState);
	}

	void pcm_c_stop()
	{
		PCM * m = PCM::getInstance();
		m->getAllCounterStates(SysAfterState, DummySocketStates, AfterState);
	}

	uint64_t pcm_c_get_cycles(uint32_t core_id)
	{
		return getCycles(BeforeState[core_id], AfterState[core_id]);
	}

	uint64_t pcm_c_get_instr(uint32_t core_id)
	{
		return getInstructionsRetired(BeforeState[core_id], AfterState[core_id]);
	}

	uint64_t pcm_c_get_core_event(uint32_t core_id, uint32_t event_id)
	{
		return getNumberOfCustomEvents(event_id, BeforeState[core_id], AfterState[core_id]);
	}
}

void build_event(const char * argv, EventSelectRegister *reg, int idx)
{
	char *token, *subtoken, *saveptr1, *saveptr2;
	char name[EVENT_SIZE], *str1, *str2;
	int j, tmp;
	uint64 tmp2;
	reg->value = 0;
	reg->fields.usr = 1;
	reg->fields.os = 1;
	reg->fields.enable = 1;

	memset(name,0,EVENT_SIZE);
	strncpy(name,argv,EVENT_SIZE-1); 
	/*
	   uint64 apic_int : 1;

	   offcore_rsp=2,period=10000
	   */
	for (j = 1, str1 = name; ; j++, str1 = NULL) {
		token = strtok_r(str1, "/", &saveptr1);
		if (token == NULL)
			break;
		printf("%d: %s\n", j, token);
		if(strncmp(token,"cpu",3) == 0)
			continue;

		for (str2 = token; ; str2 = NULL) {
			tmp = -1;
			subtoken = strtok_r(str2, ",", &saveptr2);
			if (subtoken == NULL)
				break;
			if(sscanf(subtoken,"event=%i",&tmp) == 1)
				reg->fields.event_select = tmp;
			else if(sscanf(subtoken,"umask=%i",&tmp) == 1)
				reg->fields.umask = tmp;
			else if(strcmp(subtoken,"edge") == 0)
				reg->fields.edge = 1;
			else if(sscanf(subtoken,"any=%i",&tmp) == 1)
				reg->fields.any_thread = tmp;
			else if(sscanf(subtoken,"inv=%i",&tmp) == 1)
				reg->fields.invert = tmp;
			else if(sscanf(subtoken,"cmask=%i",&tmp) == 1)
				reg->fields.cmask = tmp;
			else if(sscanf(subtoken,"in_tx=%i",&tmp) == 1)
				reg->fields.in_tx = tmp;
			else if(sscanf(subtoken,"in_tx_cp=%i",&tmp) == 1)
				reg->fields.in_txcp = tmp;
			else if(sscanf(subtoken,"pc=%i",&tmp) == 1)
				reg->fields.pin_control = tmp;
			else if(sscanf(subtoken,"offcore_rsp=%llx",&tmp2) == 1) {
				if(idx >= 2)
				{
					cerr << "offcore_rsp must specify in first or second event only. idx=" << idx << endl;
					throw idx;
				}
				events[idx].msr_value = tmp2;
			}
			else if(sscanf(subtoken,"name=%255s",events[idx].name) == 1) ;
			else
			{
				cerr << "Event '" << subtoken << "' is not supported. See the list of supported events"<< endl;
				throw subtoken;
			}

		}
	}
	events[idx].value = reg->value;
}

using namespace std;

int main(int argc, char * argv[])
{
	int m1, n1;
	m1= atoi(argv[1]);
	
	double measurements[9];

	int temp[10];

	FILE *fp;

	for (unsigned i=0;i<10;i++)
	{
		int RAND=rand();
		temp[i]=RAND;
	}
	
	PCM * pcm = PCM::getInstance();    
	if(pcm->program()!=PCM::Success) return -1;

	//L1 LOAD HIT
	build_event((char*)"umask=0x01,event=0xD1",&regs[0],0);
	//L1 LOAD MISS
	build_event((char*)"umask=0x08,event=0xD1",&regs[1],1);
	//L2 LOAD HIT
	build_event((char*)"umask=0x02,event=0xD1",&regs[2],2);
	//L2 LOAD MISS
	build_event((char*)"umask=0x10,event=0xD1",&regs[3],3);


	int big_array[FootPrint];
	srand(time(NULL));

	for(int j=0; j<FootPrint; j++)	
	{	
		int num = rand()%65535;
		big_array[j] = num;	
	}
	
	float count = 0;

	for (int k=0; k<10000; k++)
	{	
		for(int j=0; j<FootPrint; j++)	
		{	
			int num = rand()%65535;
			big_array[j] = num;	
		}
		
		//I/O
		/* Open file for both reading and writing */
		fp = fopen("file.txt", "w+");

		/* Write data to the file */
		fwrite(temp, 1+1, 1, fp);

		/* Seek to the beginning of the file*/
		fseek(fp, SEEK_SET, 0);
	
		fflush(fp);
		fsync(fileno(fp));
		fclose(fp);

		CoreCounterState before_sstate1 = getCoreCounterState(1);

		for(int j=0; j<m1; j++)
		{		
			int idx = rand()%FootPrint;
			big_array[idx]++;	
		}

		CoreCounterState after_sstate1 = getCoreCounterState(1);

/*
		measurements[0]=getInstructionsRetired(before_sstate1, after_sstate1);
		measurements[1]=getIPC(before_sstate1, after_sstate1);
		measurements[2]=getNumberOfCustomEvents(1,before_sstate1,after_sstate1);
		measurements[3]=getNumberOfCustomEvents(3,before_sstate1,after_sstate1);
		measurements[4]=getNumberOfCustomEvents(0,before_sstate1,after_sstate1);
//		measurements[5]=getL2CacheMisses(before_sstate1,after_sstate1);	
*/
		long Inst  = getInstructionsRetired(before_sstate1, after_sstate1);
		float IPC  = getIPC(before_sstate1, after_sstate1);
		int L1MISS = getNumberOfCustomEvents(1,before_sstate1,after_sstate1);
		int L2MISS = getNumberOfCustomEvents(3,before_sstate1,after_sstate1);
		int L1HIT  = getNumberOfCustomEvents(0,before_sstate1,after_sstate1);

		if (L1MISS<300){
			count++;
			measurements[0]+=Inst;
			measurements[1]+=IPC;
			measurements[2]+=L1MISS;
			measurements[3]+=L2MISS;	
		}
			
		
	}
	cout<<"with IO, # of iteration = "<<m1<<", Inst retired = "<<measurements[0]/count<<" , IPC = "<<measurements[1]/count<<" , L1Misses = "<<measurements[2]/count<<" , L2Misses = "<<measurements[3]/count<<endl;    
	return 0;
}
