#include <iostream>
#include <string>

#include "Towers.h"
#include "SoarAgent.h"

//TgDI directives
#include "TgD.h"
#include "tcl.h"

//TgDI directives
#ifdef _WIN32
#define _WINSOCKAPI_
#include <Windows.h>
#define TGD_SLEEP Sleep
#else
#include <unistd.h>
#define TGD_SLEEP usleep
#endif

#ifdef USE_GSKI_DIRECT_NOT_SML
	//gSKI directives
	#include "IgSKI_KernelFactory.h"
	#include "IgSKI_Kernel.h"
	#include "IgSKI_AgentManager.h"
	#include "IgSKI_InputProducer.h"
	#include "IgSKI_OutputProcessor.h"
	#include "IgSKI_SymbolFactory.h"
	#include "IgSKI_InputLink.h"
	#include "IgSKI_OutputLink.h"
	#include "IgSKI_WorkingMemory.h"
	#include "gSKI.h"
	#include "gSKI_Stub.h"

	using namespace gSKI;
#else
	//SML Directives
	#include "sml_Client.h"
	using namespace sml;
#endif


using std::cout; using std::cin; using std::string;

const int defaultNumTowers = 3;
const int defaultNumdisks = 11;

int main(int argc, char* argv[])
{

	cout << "***Welcome to Towers of Hanoi***" << endl << endl;

	bool doPrinting = true;
	int numTowers = defaultNumTowers;
	int numdisks = defaultNumdisks;

	if(argc > 1)
	{
		if(!strcmp(argv[1], "false"))
			doPrinting = false;
		// @TODO more checking, for robustness 
	}

	if(argc > 2)
	{
		numTowers = atoi(argv[3]);
		if(numTowers < 3)
			numTowers = 3;
	}

	//It would be flexible to read in the number of disks, but the productions are hard-coded to 11
	//if(argc > 3)
	//{
	//	numdisks = atoi(argv[3]);
	//	if(numdisks < 5)
	//		numdisks = 5; 

	//}

	// create kernel factory
#ifdef USE_GSKI_DIRECT_NOT_SML
	IKernelFactory* kFactory = gSKI_CreateKernelFactory();
#else
	//create connection for SML version
	ErrorCode error;
	Connection* pConnection = Connection::CreateEmbeddedConnection("KernelSML", &error);
	IKernelFactory* kFactory = sml_CreateKernelFactory(pConnection);
#endif

	// create kernel
	IKernel* kernel = kFactory->Create();
	IAgentManager* manager = kernel->GetAgentManager();
	gSKI::IAgent* agent = manager->AddAgent("towersAgent", "towers-of-hanoi-86.soar");


	//=============================================================================
	//========================= Setup the TgDI for the TSI ========================
	// Determine tsi version to use via command line argument
	TgD::TSI_VERSION tsiVersion;
	const char * usage = "[ 25 | 40 ]";
	if (argc > 2)
	{
		// "TgDITestd" should be argv[0] but windows puts the entire path in there
		std::cout << "Towers: too many arguments.\nUsage: towers " << usage << std::endl;
		return 1;
	} 
	else if (argc == 2)
	{
		if (strcmp(argv[1], "25") == 0)
		{
			tsiVersion = TgD::TSI25;
		}
		else if (strcmp(argv[1], "40") == 0)
		{
			tsiVersion = TgD::TSI40;
		}
		else
		{ 
			// "TgDITestd" should be argv[0] but windows puts the entire path in there
			std::cout << "towers: incorrect argument.\nUsage: towers " << usage << std::endl;
			return 1;
		}
	}
	else
	{
		std::cout << "No TSI version specified, defaulting to 4.0.0" << std::endl; 
		tsiVersion = TgD::TSI40;
	}


	//create debugger
	TgD::TgD* debugger = CreateTgD(agent, kernel, kFactory->GetKernelVersion(), tsiVersion, argv[0]);
	debugger->Init();

	//=============================================================================
	//=============================================================================


	IInputLink* iLink = agent->GetInputLink();

	HanoiWorld hanoi(iLink, doPrinting, numTowers);

	SoarAgent soarAgent(agent, &hanoi);

	//register the SoarAgent as the output processor
	IOutputLink* oLink = agent->GetOutputLink();
	oLink->AddOutputProcessor("move-disk", &soarAgent); 

	if(doPrinting)
		hanoi.Print();

int foo = 0;
while(!hanoi.AtGoalState() && foo++ < 60)
	{
		soarAgent.MakeMove();
		TgD::TgD::Update(false, debugger);

		while(TgD::TgD::Update(false, debugger))
			TGD_SLEEP(50);
	}

	if(doPrinting)
		hanoi.Print();

	while(TgD::TgD::Update(false, debugger))
		TGD_SLEEP(50);

	// Wait for the user to press return to exit the program. (So window doesn't just vanish).
	printf("\n\nPress <non-whitespace char> then enter to exit\n") ;
	string garbage;
	cin>>garbage;
	return 0;
}

