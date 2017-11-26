/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/TestILM.cpp$
$Revision: 1$
$Date: Thursday, August 14, 2008 11:09:19 AM$
$Author: benb$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  TestILM - FILE DESCRIPTION  -----------------*

Implementation of the primary LibraryModuleBase type derivation for the test
bed's primary object.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  TestILM - INCLUDES  -------------------------*/

#include "TestILM.h"
#include "CustomCommand.h"
#include <iostream>

/*-----------------  TestILM - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;
using namespace std;

class MyCustomCmd : public CustomCommand
{
public:
   virtual ProcessCmdResponse Process (String& attributes, binary& params)
   {
      cout << "In MyCustomCmd::Process" << endl;
      ProcessCmdResponse resp, resp2;
      resp.type = ProcessCmdResponse::TYPE_Success;
      resp.resultsLines.push_back(ProcessCmdResponse::Line(0, "Successful routing to the derived custom command."));
      resp2.type = ProcessCmdResponse::TYPE_Success;
      resp2.resultsLines.push_back( ProcessCmdResponse::Line(0, "ILM is invoking Tipoff."));
      LibraryModuleBase::GetHandlerObject()->Tipoff(resp2, 0);
      return resp;
   }

};

/*==========================================================================*/
   MyCustomCmd cc;
   MyCustomCmd cc2;

class TestCommand : public CustomCommand {
public:
    TestCommand() {}
    ProcessCmdResponse Process (String& attributes, binary& params) { 
        ProcessCmdResponse resp;
        resp.type = ProcessCmdResponse::TYPE_Success;
        resp.resultsLines.push_back(ProcessCmdResponse::Line(0, "Successful routing to the OASIS Test Command."));
        return resp; 
    }
};
TestCommand OASIS;


TestILM::~TestILM() {}

TestILM::TestILM() : LibraryModuleBase()
{
   cout << "Creating the TestILM instance..." << endl;

   cc.referenceID = 32;
   cc.title = "Command1";
   cc.usage = "Some usage information goes here.";
   cc.helpText = "Same with the helpText.";
   cc.cmdGroup = "Test";
   cc.attributes.push_back(CustomCommand::Attribute("string", 4));
   cc.attributes.push_back(CustomCommand::Attribute("string", 4));
   cc.parameters.push_back(CustomCommand::Parameter("G", CustomCommand::Attribute("int64", 8, "The G parameter")));
   cc.parameters.push_back(CustomCommand::Parameter("H", CustomCommand::Attribute("int32", 4, "The H parameter")));

   cout << "About to reference customCommands..." << endl;
   cout << "  it reports a size of " << customCommands.size() << endl;
   customCommands.push_back(cc);

   cc2.referenceID = 33;
   cc2.title = "Command2";
   cc2.usage = "Some other usage information goes here.";
   cc2.helpText = "Same with the other helpText.";
   cc2.cmdGroup = "Test2";
   cc2.attributes.push_back(CustomCommand::Attribute("string", 4));
   cc2.parameters.push_back(CustomCommand::Parameter("J", CustomCommand::Attribute("int64", 8, "The J parameter")));
   customCommands.push_back(cc2);

   OASIS.title = "list_modules";
   OASIS.referenceID = 100000;
   OASIS.cmdGroup = "OASIS";
   OASIS.usage = "OASIS list_modules [-t | -u | -d]";
   OASIS.helpText = "List all the loaded and unloaded modules";

   CustomCommand::Parameter param1 ("t", CustomCommand::Attribute("presence", 1, "List only temporary modules", "0") );
   CustomCommand::Parameter param2 ("u", CustomCommand::Attribute("presence", 1, "List only uninstalled modules", "0") );
   CustomCommand::Parameter param3 ("d", CustomCommand::Attribute("presence", 1, "List only deleted modules", "0") );

   OASIS.optional_parameters.push_back( param1 );
   OASIS.optional_parameters.push_back( param2 );
   OASIS.optional_parameters.push_back( param3 );

   customCommands.push_back(OASIS);
   cout << "   it now has " << customCommands.size() << " custom commands" << endl;

}

String TestILM::Initialize()
{
   cout << "Entering Initialize override..." << endl;
   return LibraryModuleBase::Initialize();
}

vector<Primitive::Validator> TestILM::ValidatePrimitives() const
{
   cout << "Entering ValidatePrimitives override..." << endl;
   return LibraryModuleBase::ValidatePrimitives();
}

const CustomCommandSet& TestILM::AddCommands() const
{
   cout << "Entering AddCommands override...\n" <<
         "   Base class reports " << LibraryModuleBase::AddCommands().size() << " custom commands." << endl;
   return LibraryModuleBase::AddCommands();
}

ProcessCmdResponse TestILM::ProcessCmd (vector<Primitive::Activation>& a)
{
   cout << "Entering ProcessCmd override..." << endl;
   return LibraryModuleBase::ProcessCmd(a);
}


