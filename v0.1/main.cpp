// archdiag v0.1 - kamor (dolfo) - 08-01-2022
// clean and sort .arc file definitions
// has a small user command interface

// use only on arch/items or other test folders in arch
// don't use it on files you want to keep human sorted (unsorted)
// script can't handle comments
// script can't handle object in object definitions
// set path to trunk and arch/item folder
// this will be stored in path.ini and arch.ini

// go first command simulate to find problems
// if getX missing definitions, update order[ORDERMAX] and ORDERMAX
// order array needs some more logic work how to sort best.
// like do lowlevel like face, animation definition first, do the most unique stuff at the end
// this could be adjusted later again, script can resort files
// if simulate looks good

// you can make first backups of .arc files using backup command
// using Sort command is real overwriting the .arc files
// if you have backup you can Restore this
// and you have option to delete all backup files

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <stdio.h> // for rename and delete files

#include <dirent.h>

#include <list> // for dynamic lists
// thx to https://www.educba.com/c-plus-plus-arraylist

using std::cout; using std::cin;
using std::string; using std::list;
using std::stringstream; using std::ifstream;

const string WHITESPACE = " \n\r\t\f\v";

#define PATH_INI  "path.ini"
#define ARCH_INI  "arch.ini"
#define PROTO_ARC "proto.arc"
#define LOG_FILE  "archdiag.log"

string path=""; // <- from path.ini or console
string arch=""; // <- from arch.ini or console

list<string> proto_order; // to store proto.arc attribute definitions, this is the sort order for engine
// list<string> :: iterator proto_it; // we don't need a global iterator

// constant definitions for mode
#define SIMULATE 0  // sort, clean .arc files without writing
#define SORT 1      // sort, clean and overwrites .arc files
#define BACKUP 2    // backup all .arc files to .arc.bak
#define DELETE 3    // delete all .bak files where .arc files is found
#define RESTORE 4   // restore all .arc.bak files
int mode = SIMULATE;
bool valid_proto = false;

std::ofstream logfile;
bool logging=false;

/*
// not used
string ltrim(const string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : s.substr(start);
}
// not used
string rtrim(const string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}
// not used
string trim(const string &s) {
    return rtrim(ltrim(s));
}
*/

// low level code to check path for directory
int DirExists(const char* filepath)
{
  struct stat info;

    if(stat( filepath, &info ) != 0)
      return 0; //invalid path
    else if(info.st_mode & S_IFDIR)
      return 1; // valid path and path is directory
    else
      return 0; // valid path, but not a directory
}
// overloaded for string type
int DirExists(string filepath)
{
  return DirExists(filepath.c_str());
}

/*int GetXold(string cmd)
{
  for (int i=0; i<ORDERMAX; i++)
  {
    // cout << "compare " << cmd << " with " << order[i] << "\n"
    if (strcmp(cmd.c_str(),order[i].c_str())==0)
    {
      // cout << "-> " << cmd << " in " << order[i] << "\n"
      return i; // return index
    }
  }
  return -1; // return error
}*/

int GetX(string cmd)
{
  list<string> :: iterator it;
  int i=0;
  for (it = proto_order.begin(); it != proto_order.end(); it++)
  {
    // cout << "compare " << cmd << " with " << *it << "\n"
    if (strcmp(cmd.c_str(),(*it).c_str())==0)
    {
      // cout << "-> " << cmd << " in " << *it << "\n"
      return i; // return index
    }
    i++;
  }
  return -1; // return error
}

// simple file copy
void copyX(string srce_file, string dest_file)
{
  std::ifstream srce(srce_file, std::ios::binary);
  if (!srce.is_open())
  {
    cout << "error : can't read "+srce_file+"\n";
    return;
  }
  else
  {
    std::ofstream dest(dest_file, std::ios::binary);
    if (!dest.is_open())
    {
     cout << "error : can't write "+dest_file+"\n";
    }
    else
    {
      dest << srce.rdbuf();
    }
    dest.close();
  }
  srce.close();
}

// core logic to work with a .arc file, can clean and reorder .arc files
int FileX(string filepath)
{
  stringstream sortX; // where we build sorted textfile

  list<string>lines; // to sort definitions
  list<string> :: iterator it; // pointer to write and read
  it=lines.begin();
  string messageblock = "";

  string line;
  ifstream myfile (filepath);
  if (myfile.is_open())
  {
    // cout << path << "\n"
    bool object=false; // a kind of mode flag to manage the object end thing
    bool last_was_end=false; // flag making one line between end and next object if there is no other
    bool message=false; // a kind of mode flag to manage the msg endmsg thing
    bool storedfirstobject=false; // flag for one line between more objects
    bool found_More=false;
    while ( getline (myfile,line) )
    {
      // line=trim(line); // no need for trim, if we use strimstream and get the words
      // cout << line << '\n';

      // extract all words from line
      stringstream in(line); // or use in<<line;
      stringstream out;
      string word;
      string cmd;
      int wordcount=0; // could be improved, we need only a flag here
      while (in >> word)
      {
        if (wordcount>0)
        {
          out << " "; // we want spaces before all words, except the first
        }
        else
        {
          cmd=word;   // first word is our command
        }
        out << word;
        wordcount++;
      }
      // to think : do we want to trim spaces in commentlines?

      // special handling for msg block, we write first to a string and insert full block to one line
      // msg block also keeps empty lines
      if (message)
      {
        messageblock+="\n"+out.str();
        if (strcmp(cmd.c_str(),"endmsg")==0)
        {
          // we have a full messageblock now
          if (it==lines.end())
          {
            lines.push_back(messageblock);
          }
          else
          {
            lines.insert(it,messageblock);
          }
          message=false; // end this mode
        }
        continue;
      }

      if (wordcount)
      {
        // if first command is Object we start collect the lines till we reach end
        if (strcmp(cmd.c_str(),"Object")==0)
        {
          if (object==true)
          {
            cout << "error : object definition without end definition in "+filepath;
            if (logging) logfile << "error : object definition without end definition in "+filepath;
            myfile.close();
            return 1;
          }

          // one extra line between end and new object, if there is no other between like #comments
          if (storedfirstobject==true)
          {
            if (last_was_end==true)
            {
              sortX << "\n";
            }
          }

          object=true;
          lines.push_front(out.str());

          found_More=false; // we reset this here

          continue;
        }

        last_was_end = false;
        if (strcmp(cmd.c_str(),"end")==0)
        {
          last_was_end = true;
          if (object!=true)
          {
            cout << "error : end definition without object definition in "+filepath;
            if (logging) logfile << "error : end definition without object definition in "+filepath;
            myfile.close();
            return 1;
          }
          lines.push_back(out.str());

          // we have a valid object now, we store it
          for (it = lines.begin(); it != lines.end(); it++)
          {
            sortX << *it << "\n";
          }

          lines.erase(lines.begin(),lines.end());
          object=false;
          storedfirstobject=true;
          continue;
        }

        // we need check object mode, if we find something not between object and end we go error
        // later we can try implement allowing comments TODO
        if (object!=true)
        {
          if (cmd.length()>0)
          {
            if (cmd[0]=='#')
            {
              sortX << out.str() << "\n";
              continue;
            }
          }

          // we allow More command
          if (strcmp(cmd.c_str(),"More")==0)
          {
            if (found_More==true)
            {
              cout << "error : double More definition in "+filepath+"\n";
              if (logging) logfile << "error : double More definition in "+filepath+"\n";
              myfile.close();
              return 1;
            }
            sortX << out.str() << "\n";
            found_More=true; // it goes false again, when we found a new object
            continue;
          }

          cout << "error : attribut definition "<< cmd <<" without object definition in "+filepath+"\n";
          if (logging) logfile << "error : attribut "<< cmd <<" definition without object definition in "+filepath+"\n";
          myfile.close();
          return 1;
        }

        // we want index for our command from order list, so we can sort it
        int index = GetX(cmd);
        if (index<0)
        {
          cout << "GetX error, index not found for " << cmd << " in " << filepath << "\n";
          if (logging) logfile << "GetX error, index not found for " << cmd << " in " << filepath << "\n";
          return 1;
        }

        // now we need to read our lines array and compare the cmd's
        bool inserted = false; // flag to know if we have found and inserted something, if not we must append it
        for(it = lines.begin(); it != lines.end(); it++)
        {
          // here we need the index of the command in lines
          // sadly we stored full line there, so we need to extract the first word again
          stringstream temp(*it);
          string tempword;
          temp >> tempword;
          int tempindex = GetX(tempword);

          // this should not be happen, but to be save
          if (tempindex<0)
          {
            cout << "GetX error, index not found for " << tempword << "in " << filepath << "\n";
            if (logging) logfile << "GetX error, index not found for " << tempword << "in " << filepath << "\n";
            return 1;
          }

          // compare //
          if(tempindex==index)
          {
            cout << "double definition " << tempword << " in " << filepath << "\n";
            if (logging) logfile << "double definition " << tempword << " in " << filepath << "\n";
            return 1;
          }

          if(tempindex<index)
          {
            continue;
          }

          // insert //

          // if we have a msg command, we don't insert, instead we store it first in a string
          if (strcmp(cmd.c_str(), "msg")==0)
          {
            messageblock=out.str();
            message=true;
            // we have iterator "it" now on the insert position for later
          }
          else
          {
            lines.insert(it, out.str());
          }
          inserted = true;
          break;
        } // for(it = lines.begin(); it != lines.end(); it++)
        // break jumps here

        // if we come from insert break iterator "it" is showing to our last write position
        // if not we set it to end of list before push_back a new line
        // so we have it always pointing to our last write position

        // push back //

        if (inserted==false)
        {
           // we also need a special logic here for msg command
          if (strcmp(cmd.c_str(), "msg")==0)
          {
            messageblock=out.str();
            message=true;
            it = lines.end(); // we set pointer to end of list, so logic knows ..
            // .. message block can't inserted, instead it must push backed
          }
          else
          {
            lines.push_back(out.str());
          }
        }
      } // if (wordcount)
    } // while (in >> word)
    myfile.close();

    if (object==true)
    {
      cout << "missing end or msgend definition in " << filepath << "\n";
      if (logging) logfile << "missing end or msgend definition in " << filepath << "\n";
      return 1;
    }

    // here we have valid, cleaned and sorted stream
    if (mode==SIMULATE)
    {
      cout << filepath << " OK!\n";
    }
    else
    {
      std::ofstream ofile(filepath.c_str());
      if (ofile.is_open())
      {
        ofile << sortX.str(); // this really overwrites file
        ofile.close();
        cout << "sorted " << filepath.c_str() << "\n";
      }
      else
      {
        cout << "error writing file " << filepath.c_str() << "\n";
        return 1;
      }
    }
  }
  else
  {
     cout << "Unable to open file : " << filepath;
     return 1;
  }
  return 0;
}

// search path, go recursive in folders, search for .arc, call FileX or do other flag commands
int DirX(string filepath)
{
  // simulate and sort needs a valid proto : todo we can go for alpabetic too?
  if (mode==SIMULATE || mode==SORT)
  {
    if (valid_proto!=true)
    {
      cout << "Not a valid proto.arc defined!\n";
      return 1;
    }
  }

  DIR *d;
  struct dirent * dir;

  d = opendir(filepath.c_str());
  if (d!=NULL)
  {
    while((dir = readdir(d)) != NULL)
    {
      if (strcmp (dir->d_name,".") == 0) continue;
      if (strcmp (dir->d_name,"..") == 0) continue;

      if (DirExists(filepath+"/"+dir->d_name))
      {
        // rekursion
        DirX(filepath+"/"+dir->d_name);
      }
      else
      {
        string name=dir->d_name; // somehow substr and size don't work with dir->name
        if (mode==RESTORE)
        {
          string tail = name.substr(name.size() - 8);
          if (strcmp(tail.c_str(),".arc.bak")!=0) continue;

          // we have .arc.bak file, we need the name without .bak

          string restore = name.substr(0,name.size()-4);
          remove((filepath+"/"+restore).c_str());
          copyX(filepath+"/"+name.c_str(),filepath+"/"+restore.c_str());
          cout << "restored " << filepath+"/"+restore << "\n";
        }

        string tail = name.substr(name.size() - 4);
        if (strcmp(tail.c_str(),".arc")!=0) continue;

        // only delete all .bak when found a .arc file before
        if (mode==DELETE)
        {
          string backup=name+".bak";
          if (remove((filepath+"/"+backup).c_str())==0)
          {
            cout << "removed " << filepath+"/"+backup << "\n";
          };
          continue;
        }
        // only backup all .arc files
        if (mode==BACKUP)
        {
          // this needs a delete before ??? TODO
          string backup=name+".bak";
          copyX(filepath+"/"+name.c_str(),filepath+"/"+backup.c_str());
          cout << filepath+"/"+backup << "\n";
          // we don't check for success, TODO
          continue;
        }
        if (mode==SIMULATE || mode==SORT)
        {
          FileX(filepath+"/"+name);
        }
      }
    } // while
  }
  closedir(d);
  return 0;
}

int goX()
{
  if (!DirExists(path))
  {
    cout << "Error (mainpath): " << path << "\n";
    cout << "Not a valid path!\n";
    return 1;
  }
  cout << "mainpath: " << path << "\n";

  string xarch=path+"/"+arch;
  if (!DirExists(xarch))
  {
    cout << "Error (archpath): " << xarch << "\n";
    return 1;
  }
  cout << "archpath: " << xarch << "\n";

  // logfile
  logfile.open(LOG_FILE);

  if (logfile.is_open())
  {
    logging=true;
  }
  else
  {
    cout << "error : could not write to " << LOG_FILE << "!\n";
    logging=false;
  }
  DirX(xarch);

  if(logging)
  {
    logfile.close();
  }
  return 0;
}

void help()
{
  cout << "(p) path : " << path << "\n";
  cout << "(a) arch : " << arch << "\n";
  cout << "(h) help\n";
  cout << "(i) init (read path, archpath and proto.arc again, if you changed from outside)\n";
  cout << "(s) simulate sort logic on .arc\n";
  // cout << "(w) where (search attribute by name in .arc\n"; // not implemented
  cout << "(b) backup .arc to .arc.bak\n";
  cout << "(d) delete .bak where .arc were found\n";
  cout << "(S) Sort .arc (overwrites .arc files)\n";
  cout << "(R) Restore .arc from .arc.bak (overwrites .arc files)\n";
  cout << "(q) quit\n";
}

void init()
{
  path = "";
  ifstream path_ini (PATH_INI); // this is in path of ArchDiag.exe
  if (path_ini.is_open())
  {
    getline (path_ini, path);
    path_ini.close();
  }
  else
  {
    cout << "no " << PATH_INI << " found!\n";
  }
  // redundant code, redundant .ini files, could be improved
  arch = "";
  ifstream arch_ini (ARCH_INI); // this is in path of ArchDiag.exe
  if (arch_ini.is_open())
  {
    getline (arch_ini, arch);
    arch_ini.close();
  }
  else
  {
    cout << "no " << ARCH_INI << " found!\n";
  }

  ifstream proto (PROTO_ARC); // this is in path of ArchDiag.exe
  string line;
  if (proto.is_open())
  {
    bool object=false;
    bool message=false;
    while (getline(proto, line))
    {
      // we only need first word
      stringstream in(line); // or use in<<line;
      string word;
      in >> word;

      // we ignore all empty lines
      if (word.length()==0)
      {
        continue;
      }

      // we ignore all #comments
      if (word[0]=='#')
      {
        continue;
      }

      if (message)
      {
        // we ignore everything till endmsg and switch back to normal mode then
        if (strcmp(word.c_str(),"endmsg")!=0)
        {
          continue;
        }
        else
        {
          message = false;
          continue;
        }
      }

      if (strcmp(word.c_str(),"Object")==0)
      {
        if (object==true)
        {
          cout << "error : object definition without end definition in " << PROTO_ARC << "!\n";
          proto.close();
          return;
        }
        proto_order.push_front("Object");
        object=true;
        continue;
      }

      if (strcmp(word.c_str(),"end")==0)
      {
        if (object!=true)
        {
          cout << "error : end definition without object definition in " << PROTO_ARC << "!\n";
          proto.close();
          return;
        }
        else
        {
          // if we find first end, we ignore rest of proto.arc
          proto_order.push_back("end");
          valid_proto=true;
          break;
        }
      }

      // we need check object mode, if we find something not between object and end we go error
      // later we can try implement allowing comments TODO
      if (object!=true)
      {
        cout << "error : attribut definition without object definition in " << PROTO_ARC << "!\n";
        proto.close();
        return;
      }
      proto_order.push_back(word);

      // special handling for msg block, we store the position of block, we ignore rest
      if (strcmp(word.c_str(),"msg")==0)
      {
        message=true;
      }
    } //while
    proto.close();
    // here all if fine
    cout << "proto definition loaded.\n";

    // list<string> :: iterator it;
    // for (it = proto_order.begin(); it != proto_order.end(); it++)
    //  cout<< *it <<'\n';

    // list<string> :: iterator proto_it;
    // for(proto_it = proto_order.begin(); proto_it != proto_order.end(); proto_it++)
    //  cout<< *proto_it <<'\n';
  }
  else
  {
    cout << "no " << PROTO_ARC << " found!\n";
  }
}

int main()
{
  cout << "ArchDiag 0.1 - 09.01.2022 - Kamor\n";
  init();
  string command;
  help();
  do
  {
    cout << "Command : ";
    cin >> command;

    if (command=="h") help();
    if (command=="p")
    {
      cout << "Path : ";
      cin >> path;

      std::ofstream file (PATH_INI); // this is path of ArchDiag.exe

      if (file.is_open())
      {
        //file.write (path.c_str(), sizeof(path.c_str())); // this overwrites a file without cleaning old content
        file << path; // this really overwrites a file
        file.close();
      }
      else
      {
        cout << "error : could not  write to " << PATH_INI << "!\n";
      }
      help();
    }
    if (command=="a")
    {
      cout << "new arch path relativ to " << path << "\n";
      cout << "arch : ";
      cin >> arch;

      std::ofstream file (ARCH_INI); // this is path of ArchDiag.exe

      if (file.is_open())
      {
        file << arch; // this really overwrites a file
        file.close();
      }
      else
      {
        cout << "error : could not  write to " << ARCH_INI << "!\n";
      }
      help();
    }

    if (command=="i")
    {
      proto_order.erase(proto_order.begin(),proto_order.end());
      init();
    }

    if (command=="s")
    {
      mode = SIMULATE;
      goX();
    }
    if (command=="b")
    {
      mode = BACKUP;
      goX();
    }
    if (command=="d")
    {
      mode = DELETE;
      goX();
    }
    if (command=="S")
    {
      mode = SORT;
      goX();
    }
    if (command=="R")
    {
      mode = RESTORE;
      goX();
    }
  }
  while (command!="q");
  return 0;
}
