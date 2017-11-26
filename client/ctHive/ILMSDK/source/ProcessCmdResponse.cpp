/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/source/ProcessCmdResponse.cpp$
$Revision: 1$
$Date: Wednesday, March 17, 2010 4:26:17 PM$
$Author: sarahs$
Template: cpp_file.cpp 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  ProcessCmdResponse - FILE DESCRIPTION  -----------------*

Implementation of basic classes and functionality used by both service and client
sides of the library interface.

*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */


/*-----------------  ProcessCmdResponse - INCLUDES  -------------------------*/

#include "ProcessCmdResponse.h"

/*-----------------  ProcessCmdResponse - DECLARATIONS ----------------------*/

using namespace InterfaceLibrary;

/*==========================================================================*/

//
// Cell class definitions
//

const string ProcessCmdResponse::Cell::row_tag("row");
const string ProcessCmdResponse::Cell::column_tag("column");
const string ProcessCmdResponse::Cell::text_tag("text");
const string ProcessCmdResponse::Cell::xmlTagName("cell");

ProcessCmdResponse::Cell::~Cell()
{}

XMLComposer&
ProcessCmdResponse::Cell::XMLCompose ( XMLComposer& dest ) const
{
   std::stringstream sc, sr;

   dest.StartTag(xmlTagName.c_str());
   sc << column;
   dest.WriteSimpleElement( column_tag.c_str(), sc.rdbuf()->str() );
   sr << row;
   dest.WriteSimpleElement( row_tag.c_str(), sr.rdbuf()->str() );
   dest.WriteSimpleElement( text_tag.c_str(), text );
   dest.EndTag();

   return dest;
}

//
// Table class definitions
//

const string ProcessCmdResponse::Table::rows_tag("rows");
const string ProcessCmdResponse::Table::columns_tag("columns");
const string ProcessCmdResponse::Table::verbosity_tag("verbosity");
const string ProcessCmdResponse::Table::xmlTagName("table");

ProcessCmdResponse::Table::~Table()
{}

XMLComposer&
ProcessCmdResponse::Table::XMLCompose ( XMLComposer& dest ) const
{
   std::stringstream sc, sr;

   dest.StartTag(xmlTagName.c_str());
   if (type.length() > 0) {
       dest.AddAttr("type", type.c_str());
   }
   if (version.length() > 0) {
       dest.AddAttr("version", version.c_str());
   }
   dest.WriteSimpleElement( verbosity_tag.c_str(), verbosity );
   sc << columns;
   dest.WriteSimpleElement( columns_tag.c_str(), sc.rdbuf()->str().c_str() );
   sr << rows;
   dest.WriteSimpleElement( rows_tag.c_str(), sr.rdbuf()->str().c_str() );
   for (size_t i=0; i<cells.size(); i++) {
      // If composeSparse is false, we'll render every cell, even the empty ones;
      // if it's true, the XML rendition can be a sparse array; no need to compose the empty cells.
      if (!composeSparse || cells[i].length()!=0)
         Cell((int)(i/columns), (int)(i%columns), cells[i]).XMLCompose(dest);
   }
   dest.EndTag();

   return dest;
}

//
// Module Show Table class definitions
//

const string ProcessCmdResponse::ModuleShowTable::name_header("Name");
const string ProcessCmdResponse::ModuleShowTable::version_header("Version");
const string ProcessCmdResponse::ModuleShowTable::bootpersistence_header("Boot Persistence");
const string ProcessCmdResponse::ModuleShowTable::state_header("State");
const string ProcessCmdResponse::ModuleShowTable::address_header("Address");
const string ProcessCmdResponse::ModuleShowTable::path_header("Path");

void  ProcessCmdResponse::ModuleShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 6);
    AddHeaderToTable();
}

void
ProcessCmdResponse::ModuleShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 6) {
        SetCellContentsAt(0, 0, name_header.c_str());
        SetCellContentsAt(0, 1, version_header.c_str());
        SetCellContentsAt(0, 2, bootpersistence_header.c_str());
        SetCellContentsAt(0, 3, state_header.c_str());
        SetCellContentsAt(0, 4, address_header.c_str());
        SetCellContentsAt(0, 5, path_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Module Show Table for header"));
    }
   
}

void
ProcessCmdResponse::ModuleShowTable::AddEntryToTable (string name, string version, bool bBootPersistence, string state, string address, string path) throw (std::runtime_error)
{
    string bootPersistence = "No";
    
    if (GetColumns() >= 6) {

        if (bBootPersistence)
            bootPersistence = "Yes";

        SetCellContentsAt(currentEntry, 0, name.c_str());
        SetCellContentsAt(currentEntry, 1, version.c_str());
        SetCellContentsAt(currentEntry, 2, bootPersistence.c_str());
        SetCellContentsAt(currentEntry, 3, state.c_str());
        SetCellContentsAt(currentEntry, 4, address.c_str());
        SetCellContentsAt(currentEntry, 5, path.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Module Show Table for module"));
    }
   
}

//
// Device Show Table class definitions
//

const string ProcessCmdResponse::DeviceShowTable::name_header("Node Name");
const string ProcessCmdResponse::DeviceShowTable::version_header("OS Version");

void  ProcessCmdResponse::DeviceShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 2);
    AddHeaderToTable();
}

void
ProcessCmdResponse::DeviceShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 2) {
        SetCellContentsAt(0, 0, name_header.c_str());
        SetCellContentsAt(0, 1, version_header.c_str());
    } else {
        throw std::runtime_error(string("Not enough columns in Device Show Table for header"));
    }
   
}

void
ProcessCmdResponse::DeviceShowTable::AddEntryToTable (string name, string version) throw (std::runtime_error)
{
    
    if (GetColumns() >= 2) {
        SetCellContentsAt(currentEntry, 0, name.c_str());
        SetCellContentsAt(currentEntry, 1, version.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Device Show Table for entry"));
    }
   
}

//
// Process Show Table class definitions
//

const string ProcessCmdResponse::ProcessShowTable::pid_header("PID");
const string ProcessCmdResponse::ProcessShowTable::process_header("Process");
const string ProcessCmdResponse::ProcessShowTable::uptime_header("Uptime");
const string ProcessCmdResponse::ProcessShowTable::uid_header("UID");
const string ProcessCmdResponse::ProcessShowTable::tty_header("TTY");
const string ProcessCmdResponse::ProcessShowTable::cmd_header("CMD");
const string ProcessCmdResponse::ProcessShowTable::parent_header("Parent");

void  ProcessCmdResponse::ProcessShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 7);
    AddHeaderToTable();
}

void
ProcessCmdResponse::ProcessShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 7) {
        SetCellContentsAt(0, 0, pid_header.c_str());
        SetCellContentsAt(0, 1, process_header.c_str());
        SetCellContentsAt(0, 2, uptime_header.c_str());
        SetCellContentsAt(0, 3, uid_header.c_str());
        SetCellContentsAt(0, 4, tty_header.c_str());
        SetCellContentsAt(0, 5, cmd_header.c_str());
        SetCellContentsAt(0, 6, parent_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Process Show Table for header"));
    }
   
}

void
ProcessCmdResponse::ProcessShowTable::AddEntryToTable (string pid, string process, string uptime, string uid, string tty, string cmd, string parent) throw (std::runtime_error)
{
    
    if (GetColumns() >= 7) {

        SetCellContentsAt(currentEntry, 0, pid.c_str());
        SetCellContentsAt(currentEntry, 1, process.c_str());
        SetCellContentsAt(currentEntry, 2, uptime.c_str());
        SetCellContentsAt(currentEntry, 3, uid.c_str());
        SetCellContentsAt(currentEntry, 4, tty.c_str());
        SetCellContentsAt(currentEntry, 5, cmd.c_str());
        SetCellContentsAt(currentEntry, 6, parent.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Process Show Table for entry"));
    }
   
}

//
// Beacon Show Table class definitions
//

const string ProcessCmdResponse::BeaconShowTable::benabled_header("Beacon Enabled");
const string ProcessCmdResponse::BeaconShowTable::initdelay_header("Initial Delay");
const string ProcessCmdResponse::BeaconShowTable::sucdelay_header("Success Delay");
const string ProcessCmdResponse::BeaconShowTable::faildelay_header("Failure Delay");
const string ProcessCmdResponse::BeaconShowTable::variance_header("Variance");
const string ProcessCmdResponse::BeaconShowTable::failsafemax_header("Failsafe Max");
const string ProcessCmdResponse::BeaconShowTable::banyenabled_header("Beacon Anyway Enabled");
const string ProcessCmdResponse::BeaconShowTable::anydelay_header("Anyway Delay");

void  ProcessCmdResponse::BeaconShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 8);
    AddHeaderToTable();
}

void
ProcessCmdResponse::BeaconShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 8) {
        SetCellContentsAt(0, 0, benabled_header.c_str());
        SetCellContentsAt(0, 1, initdelay_header.c_str());
        SetCellContentsAt(0, 2, sucdelay_header.c_str());
        SetCellContentsAt(0, 3, faildelay_header.c_str());
        SetCellContentsAt(0, 4, variance_header.c_str());
        SetCellContentsAt(0, 5, failsafemax_header.c_str());
        SetCellContentsAt(0, 6, banyenabled_header.c_str());
        SetCellContentsAt(0, 7, anydelay_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Beacon Show Table for header"));
    }
   
}

void
ProcessCmdResponse::BeaconShowTable::AddEntryToTable (bool benabled, string initdelay, string sucdelay, string faildelay, string variance, string failsafemax, bool banyenabled, string anydelay) throw (std::runtime_error)
{
    string beaconEnable = "No";
    string anywayEnable = "No";
    
    if (GetColumns() >= 8) {

        if (benabled)
            beaconEnable = "Yes";

        if (banyenabled)
            anywayEnable = "Yes";

        SetCellContentsAt(currentEntry, 0, beaconEnable.c_str());
        SetCellContentsAt(currentEntry, 1, initdelay.c_str());
        SetCellContentsAt(currentEntry, 2, sucdelay.c_str());
        SetCellContentsAt(currentEntry, 3, faildelay.c_str());
        SetCellContentsAt(currentEntry, 4, variance.c_str());
        SetCellContentsAt(currentEntry, 5, failsafemax.c_str());
        SetCellContentsAt(currentEntry, 6, anywayEnable.c_str());
        SetCellContentsAt(currentEntry, 7, anydelay.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Beacon Show Table for entry"));
    }
   
}

//
// Beacon Show Blot Slot Table class definitions
//

const string ProcessCmdResponse::BeaconShowBSTable::blotslot_header("Blot Slot");
const string ProcessCmdResponse::BeaconShowBSTable::commethod_header("Com Method");
const string ProcessCmdResponse::BeaconShowBSTable::ip_header("IP");
const string ProcessCmdResponse::BeaconShowBSTable::domain_header("Domain");
const string ProcessCmdResponse::BeaconShowBSTable::port_header("Port");

void  ProcessCmdResponse::BeaconShowBSTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 5);
    AddHeaderToTable();
}

void
ProcessCmdResponse::BeaconShowBSTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 5) {
        SetCellContentsAt(0, 0, blotslot_header.c_str());
        SetCellContentsAt(0, 1, commethod_header.c_str());
        SetCellContentsAt(0, 2, ip_header.c_str());
        SetCellContentsAt(0, 3, domain_header.c_str());
        SetCellContentsAt(0, 4, port_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Beacon Show Blot Slot Table for header"));
    }
   
}

void
ProcessCmdResponse::BeaconShowBSTable::AddEntryToTable (string blotslot, string commethod, string ip, string domain, string port) throw (std::runtime_error)
{
    
    if (GetColumns() >= 5) {
        SetCellContentsAt(currentEntry, 0, blotslot.c_str());
        SetCellContentsAt(currentEntry, 1, commethod.c_str());
        SetCellContentsAt(currentEntry, 2, ip.c_str());
        SetCellContentsAt(currentEntry, 3, domain.c_str());
        SetCellContentsAt(currentEntry, 4, port.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Beacon Show Blot Slot Table for entry"));
    }
   
}

//
// Redir Show Table class definitions
//

const string ProcessCmdResponse::RedirShowTable::rid_header("ID");
const string ProcessCmdResponse::RedirShowTable::enabled_header("Enabled");
const string ProcessCmdResponse::RedirShowTable::persist_header("Persist");
const string ProcessCmdResponse::RedirShowTable::srcip_header("Source IP");
const string ProcessCmdResponse::RedirShowTable::startsrcport_header("Start SRC Port");
const string ProcessCmdResponse::RedirShowTable::endsrcport_header("End SRC Port");
const string ProcessCmdResponse::RedirShowTable::destip_header("Dest IP");
const string ProcessCmdResponse::RedirShowTable::destmask_header("Dest Mask");
const string ProcessCmdResponse::RedirShowTable::startdestport_header("Start DST Port");
const string ProcessCmdResponse::RedirShowTable::enddestport_header("End DST Port");
const string ProcessCmdResponse::RedirShowTable::protocol_header("Protocol");
const string ProcessCmdResponse::RedirShowTable::newsrcip_header("New Source IP");
const string ProcessCmdResponse::RedirShowTable::newdestip_header("New Dest IP");
const string ProcessCmdResponse::RedirShowTable::newdestport_header("New Dest Port");
const string ProcessCmdResponse::RedirShowTable::newttl_header("New TTL");
const string ProcessCmdResponse::RedirShowTable::rule_header("Rule");

void  ProcessCmdResponse::RedirShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(16, numEntries + 1);
    AddHeaderToTable();
}

void
ProcessCmdResponse::RedirShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetRows() >= 16) {
        SetCellContentsAt(0, 0, rid_header.c_str());
        SetCellContentsAt(1, 0, enabled_header.c_str());
        SetCellContentsAt(2, 0, persist_header.c_str());
        SetCellContentsAt(3, 0, srcip_header.c_str());
        SetCellContentsAt(4, 0, startsrcport_header.c_str());
        SetCellContentsAt(5, 0, endsrcport_header.c_str());
        SetCellContentsAt(6, 0, destip_header.c_str());
        SetCellContentsAt(7, 0, destmask_header.c_str());
        SetCellContentsAt(8, 0, startdestport_header.c_str());
        SetCellContentsAt(9, 0, enddestport_header.c_str());
        SetCellContentsAt(10, 0, protocol_header.c_str());
        SetCellContentsAt(11, 0, newsrcip_header.c_str());
        SetCellContentsAt(12, 0, newdestip_header.c_str());
        SetCellContentsAt(13, 0, newdestport_header.c_str());
        SetCellContentsAt(14, 0, newttl_header.c_str());
        SetCellContentsAt(15, 0, rule_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough rows in Redir Show Table for header"));
    }
   
}

void
ProcessCmdResponse::RedirShowTable::AddEntryToTable (string rid, bool enabled, bool persist, string srcip, string startsrcport, string endsrcport, string destip, string destmask, string startdestport, string enddestport, string protocol, string newsrcip, string newdestip, string newdestport, string newttl, string rule) throw (std::runtime_error)
{
    string redirEnable = "No";
    string redirPersist = "No";
    
    if (GetRows() >= 16) {

        if (enabled)
            redirEnable = "Yes";

        if (persist)
            redirPersist = "Yes";

        SetCellContentsAt(0, currentEntry, rid.c_str());
        SetCellContentsAt(1, currentEntry, redirEnable.c_str());
        SetCellContentsAt(2, currentEntry, redirPersist.c_str());
        SetCellContentsAt(3, currentEntry, srcip.c_str());
        SetCellContentsAt(4, currentEntry, startsrcport.c_str());
        SetCellContentsAt(5, currentEntry, endsrcport.c_str());
        SetCellContentsAt(6, currentEntry, destip.c_str());
        SetCellContentsAt(7, currentEntry, destmask.c_str());
        SetCellContentsAt(8, currentEntry, startdestport.c_str());
        SetCellContentsAt(9, currentEntry, enddestport.c_str());
        SetCellContentsAt(10, currentEntry, protocol.c_str());
        SetCellContentsAt(11, currentEntry, newsrcip.c_str());
        SetCellContentsAt(12, currentEntry, newdestip.c_str());
        SetCellContentsAt(13, currentEntry, newdestport.c_str());
        SetCellContentsAt(14, currentEntry, newttl.c_str());
        SetCellContentsAt(15, currentEntry, rule.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough rows in Redir Show Table for entry"));
    }
   
}

//
// Socket Show Table class definitions
//

const string ProcessCmdResponse::SocketShowTable::sid_header("ID");
const string ProcessCmdResponse::SocketShowTable::mtu_header("MTU");

void  ProcessCmdResponse::SocketShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 2);
    AddHeaderToTable();
}

void
ProcessCmdResponse::SocketShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 2) {
        SetCellContentsAt(0, 0, sid_header.c_str());
        SetCellContentsAt(0, 1, mtu_header.c_str());
    } else {
        throw std::runtime_error(string("Not enough columns in Socket Show Table for header"));
    }
   
}

void
ProcessCmdResponse::SocketShowTable::AddEntryToTable (string sid, string mtu) throw (std::runtime_error)
{
    
    if (GetColumns() >= 2) {
        SetCellContentsAt(currentEntry, 0, sid.c_str());
        SetCellContentsAt(currentEntry, 1, mtu.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Socket Show Table for entry"));
    }
   
}

//
// Tipoff Show Table class definitions
//

const string ProcessCmdResponse::TipoffShowTable::tid_header("ID");
const string ProcessCmdResponse::TipoffShowTable::enable_header("Enable");
const string ProcessCmdResponse::TipoffShowTable::persist_header("Persist");
const string ProcessCmdResponse::TipoffShowTable::lastsent_header("Last Sent");
const string ProcessCmdResponse::TipoffShowTable::mechanism_header("Mechanism");

void  ProcessCmdResponse::TipoffShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 5);
    AddHeaderToTable();
}

void
ProcessCmdResponse::TipoffShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 5) {

        SetCellContentsAt(0, 0, tid_header.c_str());
        SetCellContentsAt(0, 1, enable_header.c_str());
        SetCellContentsAt(0, 2, persist_header.c_str());
        SetCellContentsAt(0, 3, lastsent_header.c_str());
        SetCellContentsAt(0, 4, mechanism_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show Table for header"));
    }
   
}

void
ProcessCmdResponse::TipoffShowTable::AddEntryToTable (string tid, bool enable, bool persist, string lastsent, string mechanism) throw (std::runtime_error)
{

    string toEnable = "No";
    string toPersist = "No";
    
    if (GetColumns() >= 5) {

        if (enable)
            toEnable = "Yes";

        if (persist)
            toPersist = "Yes";

        SetCellContentsAt(currentEntry, 0, tid.c_str());
        SetCellContentsAt(currentEntry, 1, toEnable.c_str());
        SetCellContentsAt(currentEntry, 2, toPersist.c_str());
        SetCellContentsAt(currentEntry, 3, lastsent.c_str());
        SetCellContentsAt(currentEntry, 4, mechanism.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show Table for entry"));
    }
   
}

//
// Tipoff Show Traffic Table class definitions
//

const string ProcessCmdResponse::TipoffShowTrafficTable::tid_header("ID");
const string ProcessCmdResponse::TipoffShowTrafficTable::delay_header("Delay");
const string ProcessCmdResponse::TipoffShowTrafficTable::srcip_header("Source IP");
const string ProcessCmdResponse::TipoffShowTrafficTable::srcmask_header("Source Mask");
const string ProcessCmdResponse::TipoffShowTrafficTable::startsrcport_header("Start SRC Port");
const string ProcessCmdResponse::TipoffShowTrafficTable::endsrcport_header("End SRC Port");
const string ProcessCmdResponse::TipoffShowTrafficTable::destip_header("Dest IP");
const string ProcessCmdResponse::TipoffShowTrafficTable::destmask_header("Dest Mask");
const string ProcessCmdResponse::TipoffShowTrafficTable::startdestport_header("Start DST Port");
const string ProcessCmdResponse::TipoffShowTrafficTable::enddestport_header("End DST Port");
const string ProcessCmdResponse::TipoffShowTrafficTable::protocol_header("Protocol");

void  ProcessCmdResponse::TipoffShowTrafficTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 11);
    AddHeaderToTable();
}

void
ProcessCmdResponse::TipoffShowTrafficTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 11) {
        SetCellContentsAt(0, 0, tid_header.c_str());
        SetCellContentsAt(0, 1, delay_header.c_str());
        SetCellContentsAt(0, 2, srcip_header.c_str());
        SetCellContentsAt(0, 3, srcmask_header.c_str());
        SetCellContentsAt(0, 4, startsrcport_header.c_str());
        SetCellContentsAt(0, 5, endsrcport_header.c_str());
        SetCellContentsAt(0, 6, destip_header.c_str());
        SetCellContentsAt(0, 7, destmask_header.c_str());
        SetCellContentsAt(0, 8, startdestport_header.c_str());
        SetCellContentsAt(0, 9, enddestport_header.c_str());
        SetCellContentsAt(0, 10, protocol_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show Traffic Table for header"));
    }
   
}

void
ProcessCmdResponse::TipoffShowTrafficTable::AddEntryToTable (string tid, string delay, string srcip, string srcmask, string startsrcport, string endsrcport, string destip, string destmask, string startdestport, string enddestport, string protocol) throw (std::runtime_error)
{
    
    if (GetColumns() >= 11) {

        SetCellContentsAt(currentEntry, 0, tid.c_str());
        SetCellContentsAt(currentEntry, 1, delay.c_str());
        SetCellContentsAt(currentEntry, 2, srcip.c_str());
        SetCellContentsAt(currentEntry, 3, srcmask.c_str());
        SetCellContentsAt(currentEntry, 4, startsrcport.c_str());
        SetCellContentsAt(currentEntry, 5, endsrcport.c_str());
        SetCellContentsAt(currentEntry, 6, destip.c_str());
        SetCellContentsAt(currentEntry, 7, destmask.c_str());
        SetCellContentsAt(currentEntry, 8, startdestport.c_str());
        SetCellContentsAt(currentEntry, 9, enddestport.c_str());
        SetCellContentsAt(currentEntry, 10, protocol.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show Traffic Table for entry"));
    }
   
}

//
// Tipoff Show User Table class definitions
//

const string ProcessCmdResponse::TipoffShowUserTable::tid_header("ID");
const string ProcessCmdResponse::TipoffShowUserTable::username_header("Username");

void  ProcessCmdResponse::TipoffShowUserTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 2);
    AddHeaderToTable();
}

void
ProcessCmdResponse::TipoffShowUserTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 2) {
        SetCellContentsAt(0, 0, tid_header.c_str());
        SetCellContentsAt(0, 1, username_header.c_str());
    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show User Table for header"));
    }
   
}

void
ProcessCmdResponse::TipoffShowUserTable::AddEntryToTable (string tid, string username) throw (std::runtime_error)
{
    
    if (GetColumns() >= 2) {
        SetCellContentsAt(currentEntry, 0, tid.c_str());
        SetCellContentsAt(currentEntry, 1, username.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Tipoff Show User Table for entry"));
    }
   
}

//
// Scramble Show Table class definitions
//

const string ProcessCmdResponse::ScrambleShowTable::availableactions_header("Available Actions");

void  ProcessCmdResponse::ScrambleShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 1);
    AddHeaderToTable();
}

void
ProcessCmdResponse::ScrambleShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 1) {
        SetCellContentsAt(0, 0, availableactions_header.c_str());
    } else {
        throw std::runtime_error(string("Not enough columns in Scramble Show Table for header"));
    }
   
}

void
ProcessCmdResponse::ScrambleShowTable::AddEntryToTable (string availableactions) throw (std::runtime_error)
{
    
    if (GetColumns() >= 1) {
        SetCellContentsAt(currentEntry, 0, availableactions.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Scramble Show Table for entry"));
    }
   
}

//
// Scramble Show User Table class definitions
//

const string ProcessCmdResponse::ScrambleShowCapabilityTable::sid_header("ID");
const string ProcessCmdResponse::ScrambleShowCapabilityTable::action_header("Action");
const string ProcessCmdResponse::ScrambleShowCapabilityTable::enabled_header("Enabled");
const string ProcessCmdResponse::ScrambleShowCapabilityTable::persist_header("Persist");

void  ProcessCmdResponse::ScrambleShowCapabilityTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 4);
    AddHeaderToTable();
}

void
ProcessCmdResponse::ScrambleShowCapabilityTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 4) {

        SetCellContentsAt(0, 0, sid_header.c_str());
        SetCellContentsAt(0, 1, action_header.c_str());
        SetCellContentsAt(0, 2, enabled_header.c_str());
        SetCellContentsAt(0, 3, persist_header.c_str());
        
    } else {
        throw std::runtime_error(string("Not enough columns in Scramble Show User Table for header"));
    }
   
}

void
ProcessCmdResponse::ScrambleShowCapabilityTable::AddEntryToTable (string sid, string action, bool enabled, bool persist) throw (std::runtime_error)
{
    string scramEnable = "No";
    string scramPersist = "No";

    if (GetColumns() >= 4) {

        if (enabled)
            scramEnable = "Yes";

        if (persist)
            scramPersist = "Yes";

        SetCellContentsAt(currentEntry, 0, sid.c_str());
        SetCellContentsAt(currentEntry, 1, action.c_str());
        SetCellContentsAt(currentEntry, 2, scramEnable.c_str());
        SetCellContentsAt(currentEntry, 3, scramPersist.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Scramble Show User Table for entry"));
    }
   
}

//
// Capability Show Table class definitions
//

const string ProcessCmdResponse::CapabilityShowTable::name_header("Name");
const string ProcessCmdResponse::CapabilityShowTable::middle_header("");
const string ProcessCmdResponse::CapabilityShowTable::version_header("Version");


void  ProcessCmdResponse::CapabilityShowTable::InitializeTable (int numEntries) throw (std::runtime_error)
{
    SetSize(numEntries+1, 3);
    AddHeaderToTable();
}

void
ProcessCmdResponse::CapabilityShowTable::AddHeaderToTable() throw (std::runtime_error)
{

    if (GetColumns() >= 3) {
        SetCellContentsAt(0, 0, name_header.c_str());
        SetCellContentsAt(0, 1, middle_header.c_str());
        SetCellContentsAt(0, 2, version_header.c_str());

    } else {
        throw std::runtime_error(string("Not enough columns in Capability Show Table for header"));
    }
   
}

void
ProcessCmdResponse::CapabilityShowTable::AddEntryToTable (string name, string middle, string version) throw (std::runtime_error)
{
    if (GetColumns() >= 3) {

        SetCellContentsAt(currentEntry, 0, name.c_str());
        SetCellContentsAt(currentEntry, 1, middle.c_str());
        SetCellContentsAt(currentEntry, 2, version.c_str());

        currentEntry++;

    } else {
        throw std::runtime_error(string("Not enough columns in Capability Show Table for entry"));
    }
   
}


//
// Line class definitions
//

const string ProcessCmdResponse::Line::text_tag("text");
const string ProcessCmdResponse::Line::verbosity_tag("verbosity");
const string ProcessCmdResponse::Line::xmlTagName("line");

ProcessCmdResponse::Line::~Line()
{
}

XMLComposer&
ProcessCmdResponse::Line::XMLCompose (XMLComposer& dest) const
{
   dest.StartTag(xmlTagName.c_str());
   dest.WriteSimpleElement( verbosity_tag.c_str(), (long)verbosity );
   dest.WriteSimpleElement( text_tag.c_str(), text );
   dest.EndTag();

   return dest;
}


//
// ProcessCmdResponse class definitions
//


const string ProcessCmdResponse::type_tag("type");
const string ProcessCmdResponse::cmdid_tag("cmdid");
const string ProcessCmdResponse::results_tag("results");
const string ProcessCmdResponse::xmlTagName("return");
const char* const ProcessCmdResponse::TYPE_Success = "Success";
const char* const ProcessCmdResponse::TYPE_Pending = "Pending";
const char* const ProcessCmdResponse::TYPE_Local_Failure = "Local Failure";
const char* const ProcessCmdResponse::TYPE_Remote_Failure = "Remote Failure";

ProcessCmdResponse::~ProcessCmdResponse()
{}

XMLComposer&
ProcessCmdResponse::XMLCompose (XMLComposer& dest) const
{
   dest.StartTag(xmlTagName.c_str());
   dest.WriteSimpleElement( type_tag.c_str(), type );
   dest.WriteSimpleElement( cmdid_tag.c_str(), (long)cmdid);
   dest.StartTag(results_tag.c_str());
   for (std::vector<Table>::const_iterator i = resultsTables.begin(); i!=resultsTables.end(); i++)
      i->XMLCompose(dest);
   for (std::vector<Line>::const_iterator j = resultsLines.begin(); j!=resultsLines.end(); j++)
      j->XMLCompose(dest);
   dest.EndTag();
   dest.EndTag();

   return dest;
}


