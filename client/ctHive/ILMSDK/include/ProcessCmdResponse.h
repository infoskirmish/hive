/*-****************************************************************************
$Archive: SinnerTwin/JY008C637-ILM_SDK/include/ProcessCmdResponse.h$
$Revision: 1$
$Date: Wednesday, March 17, 2010 4:26:17 PM$
$Author: sarahs$
Template: cpp_file.h 3.0
CPRCLASS = "PROPRIETARY LEVEL I"
*******************************************************************************
*/

/*-----------------  ProcessCmdResponse - FILE DESCRIPTION  -----------------*

Declaration of the ProcessCmdResponse class and the component classes that are
part of its modeling.
*/
/* $NoKeywords$ (No rcs replacement keywords below this point) */

#if !defined(PROCESSCMDRESPONSE_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)
#define PROCESSCMDRESPONSE_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_

/*-----------------  ProcessCmdResponse - INCLUDES  -------------------------*/

#include <vector>
#include <stdexcept>
#include "SDKDataTypes.h"
#include "XMLComposer.h"

/*-----------------  ProcessCmdResponse - MISCELLANEOUS  --------------------*/

#define MODULESHOWTABLE_VERSION "1"
#define DEVICESHOWTABLE_VERSION "1"
#define PROCESSSHOWTABLE_VERSION "1"
#define BEACONSHOWTABLE_VERSION "1"
#define BEACONSHOWBSTABLE_VERSION "1"
#define REDIRSHOWTABLE_VERSION "1"
#define SOCKETSHOWTABLE_VERSION "1"
#define TIPOFFSHOWTABLE_VERSION "1"
#define TIPOFFSHOWTRAFFICTABLE_VERSION "1"
#define TIPOFFSHOWUSERTABLE_VERSION "1"
#define SCRAMBLESHOWTABLE_VERSION "1"
#define SCRAMBLESHOWCAPABILITYTABLE_VERSION "1"
#define CAPABILITYSHOWTABLE_VERSION "1"

using std::string;

/*-----------------  ProcessCmdResponse - DECLARATIONS  ---------------------*/

namespace InterfaceLibrary {

/**
The ProcessCmdResponse class models the reply from a ProcessCmd call as
described in JY008C615, the CutThroat ICD.  An object of this type has the
ability to serialize itself into an XML text stream as required by that
interface definition.

In an ILM, a ProcessCmdResponse is typically populated, for the most part, by
the handler functions for the command primitives that are invoked through
arguments of the ProcessCmd call of the CT/ILM interface.  The default
ProcessCmd method implementation supplied by the LibraryModuleBase class will
instantiate a ProcessCmdResponse, and will then hand a reference to that object
in turn to the handler function for each of the primitives.  Each handler then
has an opportunity to add its contribution to the response; that contribution is
in the form of one or more Table or Line objects, both of which are subtypes of
ProcessCmdResponse.  <b>Thus, if this default behavior of LibraryModuleBase is
not overridden, the ILM implementer will probably not need to declare a
ProcessCmdResponse object.</b>

Note that ProcessCmdResponse is not big on data hiding; all the members are
public.  Most are constant, though, except for the Table and Line containers,
but since those are standard template types, there really isn't much you can
do to mess them up.  The only thing the developer has to be sure of is that the
type member, which is a std::string, is set to one of the four constant members
defined for this purpose.
*/
class ProcessCmdResponse
{
public:

    ProcessCmdResponse(){
        cmdid = 0;
    }

   /// The virtual destructor doesn't explicitly do anything, but is supplied for extensibility.
   virtual
   ~ProcessCmdResponse();

   /**
   This method causes the ProcessCmdResponse object to serialize itself into the
   specified XML text stream composer.

   <returns>  The dest argument into which the object's XML rendition was
   added. </returns>
   */
   virtual XMLComposer&
   XMLCompose (
      XMLComposer&   dest
      ) const;

   /**
      The Cell class models the "cell" sub-element of a Table as described in
      the CutThroat ICD's schema for the ProcessCmd response. A Cell is simply a
      text string at a known position (row and column) within a Table; row and
      column positions are zero-based by convention.
   */
   class Cell
   {
   public:

      /**
      Default constructor for a Cell.  The resulting object is invalid (i.e.,
      has invalid row and column positions) until its position in the Table is
      defined.
      */
      Cell()
      { row=column=-1; }

      /**
      Explicit member initializer constructor for a Cell

      <param name="r"> The row position initializer; must be non-negative to
            create a valid Cell object.  </param>
      <param name="c"> The column position initializer; must be non-negative to
            create a valid Cell object.  </param>
      <param name="t"> The text content initializer. </param>
      */
      Cell (
         int r,
         int c,
         string t
         )
      {
         row = r;
         column = c;
         text = t;
      }

      /**
      Copy constructor for a Cell
      */
      Cell(
         const Cell& c
         )
      {
         row=c.row;
         column=c.column;
         text=c.text;
      }

      virtual
      ~Cell();

      /**
      Causes the Cell to serialize itself into the
      specified XML text stream composer.

      <returns>  The dest argument into which the object's XML rendition was
      added. </returns>
      */
      virtual XMLComposer&
      XMLCompose (
         XMLComposer&   dest
         ) const;

      // The "row" tag.
      static const string row_tag;
      // The "column" tag.
      static const string column_tag;
      // The "text" tag.
      static const string text_tag;
      // The "cell" element tag.
      static const string xmlTagName;

      /// The row in which this Cell resides.  Zero-based; negative values indicate an uninitialized object.
      int      row;
      /// The column in which this Cell resides.  Zero-based; negative values indicate an uninitialized object.
      int      column;
      /// The text contents of the Cell.
      string   text;
   };

   /**
      The Table class can occur multiple times within a ProcessCmd response, and logically
      consists of a two-dimensional array of Cells, with the dimensions of the array
      given by the rows and columns members.  In reality, since a Table physically
      stores itself as a non-sparse array, the position information in the Cells
      is redundant, so the use of the Cell class is used mainly to assist in
      serialization.
   */
   class Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      Table ()
         : cells(0)
      {
         rows = columns = 0;
         currentEntry = 1;
         type = "";
         version = "";
         composeSparse = true;
      }

      /**
         Copy constructor for a results Table.
      */
      Table (
         const Table&   t
         )
         : verbosity(t.verbosity), cells(t.cells)
      {
         rows = t.rows;
         columns = t.columns;
         composeSparse = t.composeSparse;
      }

      virtual
      ~Table();

      /**
         Causes the Table to serialize itself into the
         specified XML text stream composer.

         <returns>  The dest argument into which the object's XML rendition was
         added. </returns>
      */
      virtual XMLComposer&
      XMLCompose (
         XMLComposer&   dest
         ) const;

      /**
         Returns the number of rows in the Table.
      */
      int   GetRows() const
      { return rows; }

      /**
         Returns the number of columns in the Table.
      */
      int   GetColumns() const
      { return columns; }

      /**
         Sets the dimensions of the Table.  Previous contents of the table
         are discarded by this method.
      */
      virtual void  SetSize (int numRows, int numColumns)
      {
         cells.clear();
         if (numRows>0 && numColumns>0) {
            rows = numRows;
            columns = numColumns;
            cells.resize(numRows*numColumns);
         }
      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      virtual void  AddHeaderToTable () throw (std::runtime_error) {}

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      virtual void  InitializeTable (int numEntries) throw (std::runtime_error){}

      /**
         Returns the Cell at the specified location in the table; row and col are zero-based.
      */
      Cell GetCellAt (int row, int col) const
      {
         return Cell(row, col, cells[row*columns+col]);
      }

      /**
         Returns the text contents of the Cell at the specified location in the table; row
         and col are zero-based.
      */
      const string&  GetCellContentsAt (int row, int col) const
      { return cells[row*columns+col]; }

      /**
         Replaces the text contents of the Cell at the specified location in the table; row
         and col are zero-based.
      */
      string&  SetCellContentsAt (int row, int col, const char* newText)
      { return (cells[row*columns+col]=newText); }

      /// The simple string sub-element of the same name that appears in the response.
      string   verbosity;

      /// When true, an XML serialization of the Table will not include empty cells.  True by default.
      bool     composeSparse;

      static const string rows_tag;
      static const string columns_tag;
      static const string verbosity_tag;

      /// The "table" element name
      static const string xmlTagName;

   protected:

       /// The type of table
       string type;

       /// The version of this type of table
       string version;

      /// Number of rows in the table.  A non-positive value indicates an uninitialzed object.
      int      rows;

      /// Number of columns in the table.  A non-positive value indicates an uninitialzed object.
      int      columns;

      /// The counter to keep track of the current entry position in the table
      int currentEntry;

      /// A row-wise arrangement of the table's cell contents.  The size will be
      /// equal to (rows*columns) when those members are valid (i.e., greater than zero.)
      std::vector<string> cells;

   };

   /**
      Barely qualifying as a "complex type", a Line is an element containing
      verbosity and text sub-elements, both simple strings.
   */
   class Line
   {
   public:
      /**
         Default constructor for a Line.
      */
      Line()
      {}

      /**
         Copy constructor for a Line.
      */
      Line(const Line& n)
         : verbosity(n.verbosity), text(n.text)
      {}

      /**
         Initializing constructor for a Line.
      */
      Line( uint32 verb, const char* txt )
         : verbosity(verb), text(txt)
      {}

      virtual
      ~Line();

      /**
         Causes the Line to serialize itself into the
         specified XML text stream composer.

      Returns:
         The dest argument.
      */
      virtual XMLComposer&
      XMLCompose (
         XMLComposer&   dest
         ) const;

      static const string text_tag;
      static const string verbosity_tag;
      /// The "line" tag name
      static const string xmlTagName;

      uint32   verbosity;
      string   text;
   };

   /**
   *  The ModuleShowTable is derived from the Table class. This
   *  class is used to create the expected output for the Module
   *  Show command.
   *  * */
   class ModuleShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      ModuleShowTable ()
      {
         rows = columns = 0;

         type = "ModuleShowTable";
         version = MODULESHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="name"> The name of the module.</param>
      
      <param name="version"> The version of the module.</param>
      
      
      <param name="bBootPersistence"> Flag indicating whether or not the module will
      execute automatically on reboot.</param>

      <param name="state"> The current state of the module.  The four options are:
      Running, Stopped, Waiting, and Failed</param>

      <param name="address"> The location in RAM where the module exists.</param>

      <param name="path"> The location in persistent storage where the module exists.</param>
        
      */ 

      virtual void  AddEntryToTable (string name, string version, bool bBootPersistence, string state, string address, string path) throw (std::runtime_error);

      /// The name column header of the table
      static const string name_header;

      /// The version column header of the table
      static const string version_header;

      /// The boot persistence column header of the table
      static const string bootpersistence_header;

      /// The state column header of the table
      static const string state_header;

      /// The address column header of the table
      static const string address_header;

      /// The path column header of the table
      static const string path_header;
   };

   /**
   *  The DeviceShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Device Show command.
   *  * */
   class DeviceShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      DeviceShowTable ()
      {
         rows = columns = 0;

         type = "DeviceShowTable";
         version = DEVICESHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="name"> The name of the tool.</param>
      
      <param name="version"> The os version of the target.</param>
      
        
      */ 

      virtual void  AddEntryToTable (string name, string version) throw (std::runtime_error);

      /// The name column header of the table
      static const string name_header;

      /// The version column header of the table
      static const string version_header;

   };

   /**
   *  The ProcessShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Process Show command.
   *  * */
   class ProcessShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      ProcessShowTable ()
      {
         rows = columns = 0;

         type = "ProcessShowTable";
         version = PROCESSSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="pid"> The process id </param>
      
      <param name="process"> The process name </param>
      
      <param name="uptime"> The amount of time the process has
      been executing </param>
      
      <param name="uid"> User id associated with the process
      </param>

      <param name="tty"> The terminal associated with the
      process</param>

      <param name="cmd"> The command associated with the
      process</param>

      <param name="parent"> The process id of this processes
      parent</param>
        
      */ 

      virtual void  AddEntryToTable (string pid, string process, string uptime, string uid, string tty, string cmd, string parent) throw (std::runtime_error);

      /// The pid column header of the table
      static const string pid_header;

      /// The process column header of the table
      static const string process_header;

      /// The uptime column header of the table
      static const string uptime_header;

      /// The uid column header of the table
      static const string uid_header;

      /// The tty column header of the table
      static const string tty_header;

      /// The cmd column header of the table
      static const string cmd_header;

      /// The parent column header of the table
      static const string parent_header;

   };

   /**
   *  The BeaconShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Beacon Show command.
   *  * */
   class BeaconShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      BeaconShowTable ()
      {
         rows = columns = 0;

         type = "BeaconShowTable";
         version = BEACONSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="benabled"> The current state of the beacon
      capability </param>
      
      <param name="initdelay"> The time, in seconds, before the
      first beacon is sent </param>
      
      <param name="sucdelay"> The time, in seconds, between beacons
      after a successful beacon</param>
      
      <param name="faildelay"> The time, in seconds, between beacons
      after a failed beacon</param>
      
      <param name="variance"> The variance, in seconds,
      applied to the beacon interval between beacons after a
      failed beacon</param>

      <param name="failsafemax"> The maximum number of times the
      beacon can fail before it stops attempting. </param>

      <param name="banyenabled"> The current state of the
      beacon anyway setting. This makes the tool beacon using the
      hosted devices IP when it can't find a suitable ip to spoof.
      </param>
      
      <param name="anydelay"> The time, in seconds, before an
      anyway beacon is sent. </param>
        
      */ 

      virtual void  AddEntryToTable (bool benabled, string initdelay, string sucdelay, string faildelay, string variance, string failsafemax, bool banyenabled, string anydelay) throw (std::runtime_error);

      /// The Beacon Enabled column header of the table
      static const string benabled_header;

      /// The Initial Delay column header of the table
      static const string initdelay_header;

      /// The Success Delay column header of the table
      static const string sucdelay_header;

      /// The Failure Delay column header of the table
      static const string faildelay_header;

      /// The Variance column header of the table
      static const string variance_header;

      /// The Failsafe Max header of the table
      static const string failsafemax_header;

      /// The Beacon Anyway Enabled column header of the table
      static const string banyenabled_header;

      /// The Anyway Delay column header of the table
      static const string anydelay_header;

   };

   /**
   *  The BeaconShowBSTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Beacon Show command, specifically the Blot Slot table.
   *  * */
   class BeaconShowBSTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      BeaconShowBSTable ()
      {
         rows = columns = 0;

         type = "BeaconShowBSTable";
         version = BEACONSHOWBSTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="blotslot"> The drop point configuration
      profile</param>
      
      <param name="commethod"> The communication protocol*
      </param>
      
      <param name="ip"> The ip address to be used
      </param>
      
      <param name="domain"> The domain name to be used
      </param>
      
      <param name="port"> The port to be used </param>
        
      */ 

      virtual void  AddEntryToTable (string blotslot, string commethod, string ip, string domain, string port) throw (std::runtime_error);

      /// The Blot Slot column header of the table
      static const string blotslot_header;

      /// The Com Method column header of the table
      static const string commethod_header;

      /// The IP column header of the table
      static const string ip_header;

      /// The Domain column header of the table
      static const string domain_header;

      /// The Port column header of the table
      static const string port_header;

   };

   /**
   *  The RedirShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Redir Show command.
   *  * */
   class RedirShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      RedirShowTable ()
      {
         rows = columns = 0;

         type = "RedirShowTable";
         version = REDIRSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="rid"> A string that uniquely identifies the*
      redirecion rule </param>
      
      <param name="enabled"> The current operational status of the
      redirection</param>
      
      <param name="persist"> The redirection rules ability to survive reboots </param>
      
      <param name="srcip"> The source ip this rule filters on </param>
      
      <param name="startsrcport"> The initial source port in the range this rule filters on </param>

      <param name="endsrcport"> The final source port in the range this rule filters on </param>

      <param name="destip"> The destination ip this rule filters on </param>
      
      <param name="destmask"> The mask applied to the destination ips </param>
        
      <param name="startdestport"> The initial destination port in the range this rule filters on </param>

      <param name="enddestport"> The final destination port in the range this rule filters on </param>

      <param name="protocol"> The protocol this rule filters on </param>
      
      <param name="newsrcip"> The new source ip </param>
        
      <param name="newdestip"> The new destination ip </param>

      <param name="newdestport"> The new destination port </param>

      <param name="newttl"> The new time to live value </param>

      <param name="rule"> </param>
      */

      virtual void  AddEntryToTable (string rid, bool enabled, bool persist, string srcip, string startsrcport, string endsrcport, string destip, string destmask, string startdestport, string enddestport, string protocol, string newsrcip, string newdestip, string newdestport, string newttl, string rule) throw (std::runtime_error);

      /// The ID row header of the table
      static const string rid_header;

      /// The Enabled row header of the table
      static const string enabled_header;

      /// The Persist row header of the table
      static const string persist_header;

      /// The Source IP row header of the table
      static const string srcip_header;

      /// The Start Source Port row header of the table
      static const string startsrcport_header;

      /// The End Source Port row header of the table
      static const string endsrcport_header;

      /// The Destination IP row header of the table
      static const string destip_header;

      /// The Destination Mask row header of the table
      static const string destmask_header;

      /// The Start Destination Port row header of the table
      static const string startdestport_header;

      /// The End Destination Port row header of the table
      static const string enddestport_header;

      /// The Protocol row header of the table
      static const string protocol_header;

      /// The New Source IP row header of the table
      static const string newsrcip_header;

      /// The New Destination IP row header of the table
      static const string newdestip_header;

      /// The New Destination Port row header of the table
      static const string newdestport_header;

      /// The New TTL row header of the table
      static const string newttl_header;

      /// The Rule row header of the table
      static const string rule_header;

   };

   /**
   *  The SocketShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Socket Show command.
   *  * */
   class SocketShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      SocketShowTable ()
      {
         rows = columns = 0;

         type = "SocketShowTable";
         version = SOCKETSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="sid"> A number that identifies the
      socket</param>
      
      <param name="mtu"> Maximum Transmission Unit in
      bytes</param>
      
        
      */ 

      virtual void  AddEntryToTable (string sid, string mtu) throw (std::runtime_error);

      /// The ID column header of the table
      static const string sid_header;

      /// The MTU column header of the table
      static const string mtu_header;

   };

   /**
   *  The TipoffShowTable class is derived from the Table class.
   *  This class is used to create the expected output for the
   *  Tipoff Show command.
   *  * */
   class TipoffShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      TipoffShowTable ()
      {
         rows = columns = 0;

         type = "TipoffShowTable";
         version = TIPOFFSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="tid"> A string that uniquely identifies the tipoff*
      event </param>**
      
      <param name="enable"> The current operational status of the tipoff </param>
      
      <param name="persist"> The tipoff rules ability to survive reboots </param>
      
      <param name="lastsent"> The timestamp of the last time this tipoff was sent </param>
      
      <param name="mechanism"> The mechanism used to communicate the tipoff. CP stands for Control Post communications.
          DP stands for Drop Point communications. </param>
      */  

      virtual void  AddEntryToTable (string tid, bool enable, bool persist, string lastsent, string mechanism) throw (std::runtime_error);

      /// The ID column header of the table
      static const string tid_header;

      /// The Enable column header of the table
      static const string enable_header;

      /// The Persist column header of the table
      static const string persist_header;

      /// The Last Sent column header of the table
      static const string lastsent_header;

      /// The Mechanism column header of the table
      static const string mechanism_header;

   };

   /**
   *  The TipoffShowTrafficTable class is derived from the Table
   *  class. This class is used to create the expected output for
   *  the Tipoff Show command, in particular the traffic filters.
   *  * */
   class TipoffShowTrafficTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      TipoffShowTrafficTable ()
      {
         rows = columns = 0;

         type = "TipoffShowTrafficTable";
         version = TIPOFFSHOWTRAFFICTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.

      <param name="tid"> A string that uniquely identifies the tipoff*
      event </param>**
      
      <param name="delay"> They amount of time after a matching traffic is detected before a 
          tipoff is sent </param>
      
      <param name="srcip"> The source ip address this rule filters on </param>
      
      <param name="srcmask"> The source mask this rule filters on </param>
      
      <param name="startsrcport"> The initial source port in the range this rule filters on </param>

      <param name="endsrcport"> The final source port in the range this rule filters on </param>

      <param name="destip"> The destination ip this rule filters on </param>
      
      <param name="destmask"> The mask applied to the destination ips </param>
        
      <param name="startdestport"> The initial destination port in the range this rule filters on </param>

      <param name="enddestport"> The final destination port in the range this rule filters on </param>

      <param name="protocol"> The protocol this rule filters on </param>
        
      */ 

      virtual void  AddEntryToTable (string tid, string delay, string srcip, string srcmask, string startsrcport, string endsrcport, string destip, string destmask, string startdestport, string enddestport, string protocol) throw (std::runtime_error);

      /// The ID column header of the table
      static const string tid_header;

      /// The Delay column header of the table
      static const string delay_header;

      /// The Source IP column header of the table
      static const string srcip_header;

      /// The Source Mask column header of the table
      static const string srcmask_header;

      /// The Start Source Port row header of the table
      static const string startsrcport_header;

      /// The End Source Port row header of the table
      static const string endsrcport_header;

      /// The Destination IP row header of the table
      static const string destip_header;

      /// The Destination Mask row header of the table
      static const string destmask_header;

      /// The Start Destination Port row header of the table
      static const string startdestport_header;

      /// The End Destination Port row header of the table
      static const string enddestport_header;

      /// The Protocol row header of the table
      static const string protocol_header;

   };

   /**
   *  The TipoffShowUserTable class is derived from the Table
   *  class. This class is used to create the expected output for
   *  the Tipoff Show command, in particular user alerts.
   *  * */
   class TipoffShowUserTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      TipoffShowUserTable ()
      {
         rows = columns = 0;

         type = "TipoffShowUserTable";
         version = TIPOFFSHOWUSERTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="tid"> A string that uniquely identifies the tipoff event </param>
      
      <param name="username"> The name of the user whose logon triggers the tipoff </param>
      
      */  

      virtual void  AddEntryToTable (string tid, string username) throw (std::runtime_error);

      /// The ID column header of the table
      static const string tid_header;

      /// The Username column header of the table
      static const string username_header;

   };

   /**
   *  The ScrambleShowTable class is derived from the Table
   *  class. This class is used to create the expected output for
   *  the Scramble Show command.
   *  * */
   class ScrambleShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      ScrambleShowTable ()
      {
         rows = columns = 0;

         type = "ScrambleShowTable";
         version = SCRAMBLESHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="availableactions"> A string that describes what scramble capabilities are available </param>
      
      */
      
      virtual void  AddEntryToTable (string availableactions) throw (std::runtime_error);

      /// The Available Actions column header of the table
      static const string availableactions_header;

   };

   /**
   *  The ScrambleShowCapabilityTable class is derived from the
   *  Table class. This class is used to create the expected output
   *  for the Scramble Show command, in particular the
   *  capabilities.
   *  * */
   class ScrambleShowCapabilityTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      ScrambleShowCapabilityTable ()
      {
         rows = columns = 0;

         type = "ScrambleShowCapabilityTable";
         version = SCRAMBLESHOWCAPABILITYTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="sid"> A string that uniquely identifies the scramble rule </param>
      
      <param name="action"> The action this rule takes </param>

      <param name="enabled"> Whether or not the rule is enabled </param>

      <param name="persist"> The tipoff rules ability to survive reboots </param>
      
      */ 

      virtual void  AddEntryToTable (string sid, string action, bool enabled, bool persist) throw (std::runtime_error);

      /// The ID column header of the table
      static const string sid_header;

      /// The Action column header of the table
      static const string action_header;

      /// The Enabled column header
      static const string enabled_header;

      /// The Persist column header of the table
      static const string persist_header;

   };


   /**
   *  The CapabilityShowTable is derived from the Table class. This
   *  class is used to create the expected output for the Capability
   *  Show command.
   *  * */
   class CapabilityShowTable : public Table
   {
   public:
      /**
         Default constructor for a results Table.
      */
      CapabilityShowTable ()
      {
         rows = columns = 0;

         type = "CapabilityShowTable";
         version = CAPABILITYSHOWTABLE_VERSION;

         composeSparse = true;

      }

      /**
      *  Adds a header row to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must
      *  be called to set the size of the table before anything is
      *  added to the table.
        
      */ 

      void  AddHeaderToTable () throw (std::runtime_error);

      /**
      *  Sets the size of the table and adds the header.
      * 
      *  <param name="numEntries"> The number of entries in this
      *  table.</param>
        
      */ 

      void  InitializeTable (int numEntries) throw (std::runtime_error);

      /**
      *  Adds an entry to the table that will be returned with the
      *  command response.  Note, the Table::SetSize function must be
      *  called to set the size of the table before anything is added
      *  to the table.
      
      <param name="name"> The name of the module.</param>
      
      <param name="middle"> Either blank (for non-verbose responses), 
      or a sub-component name.</param> 
      
      <param name="version"> The version of the module.</param>
              
      */ 

      virtual void  AddEntryToTable (string name, string middle, string version) throw (std::runtime_error);

      /// The name column header of the table
      static const string name_header;

      /// The column header for the middle column of the table - blank
      static const string middle_header;

      /// The version column header of the table
      static const string version_header;

   };

   /// This is the overall response type for the command.  The ICD states that this
   /// string must contain one of four values: TYPE_Success, TYPE_Pending, TYPE_Local_Failure,
   /// or TYPE_Remote_Failure.
   string   type;

   /// This is the unique identifier for this command.  This will be used to relate pending
   /// commands with their results.
   uint32 cmdid;

   /// The set of Tables in the response.
   std::vector<Table>   resultsTables;

   /// The set of Lines in the response.
   std::vector<Line>    resultsLines;

   /// The UML tag for the 'type' element.
   static const string type_tag;

   /// The UML tag for the 'cmdid' element.
   static const string cmdid_tag;

   /// The UML tag for the 'results' element.
   static const string results_tag;

   /// The "return" tag for the ProcessCmd response's root element.
   static const string xmlTagName;

   /// Used in the type member to indicate an overall successful conclusion of command processing.
   static const char *const TYPE_Success;

   /// Used in the type member to indicate that processing is continuing in the background.
   static const char* const TYPE_Pending;

   /// Used in the type member to indicate a failure in processing at the Control Post end.
   static const char* const TYPE_Local_Failure;

   /// Used in the type member to indicate a failure in processing at the remote end.
   static const char* const TYPE_Remote_Failure;

};

};

#endif // !defined(PROCESSCMDRESPONSE_H__36174FA8_C9CA_4d46_BB20_5DF0AB9A4655__INCLUDED_)
