#include "Parameters.h"
#include "ErrorWarning.h"
#include <fstream>
#include <sys/stat.h>
void Parameters::openReadsFiles() 
{
    if (readFilesCommandString=="") {//read from file
        for (uint ii=0;ii<readFilesIn.size();ii++) {//open readIn files
            readFilesCommandPID[ii]=0;//no command process IDs
            if ( inOut->readIn[ii].is_open() ) inOut->readIn[ii].close();

            string rfName=readFilesPrefixFinal + readFilesIn.at(ii);

            inOut->readIn[ii].open(rfName.c_str()); //try to open the Sequences file right away, exit if failed
            if (inOut->readIn[ii].fail()) {
                ostringstream errOut;
                errOut <<"EXITING because of fatal input ERROR: could not open readFilesIn=" << rfName <<"\n";
                exitWithError(errOut.str(), std::cerr, inOut->logMain, EXIT_CODE_PARAMETER, *this);
            };
        };
    } else {//create fifo files, execute pre-processing command

         vector<string> readsCommandFileName;

         for (uint imate=0;imate<readFilesIn.size();imate++) {//open readIn files
            ostringstream sysCom;
            sysCom << outFileTmp <<"tmp.fifo.read"<<imate+1;
            readFilesInTmp.push_back(sysCom.str());
            remove(readFilesInTmp.at(imate).c_str());
            mkfifo(readFilesInTmp.at(imate).c_str(), S_IRUSR | S_IWUSR );

            inOut->logMain << "\n   Input read files for mate "<< imate+1 <<" :\n";

            readsCommandFileName.push_back(outFileTmp+"/readsCommand_read" + to_string(imate+1));
            fstream readsCommandFile( readsCommandFileName.at(imate).c_str(), ios::out);
            readsCommandFile.close();
            readsCommandFile.open( readsCommandFileName.at(imate).c_str(), ios::in | ios::out);
            //first line in the commands file
            if (sysShell!="-") {//executed via specified shell
                readsCommandFile << "#!" <<sysShell <<"\n";
            };
            readsCommandFile << "exec > \""<<readFilesInTmp.at(imate)<<"\"\n" ; // redirect stdout to temp fifo files

            for (uint32 ifile=0; ifile<readFilesN; ifile++) {
                
                system(("ls -lL " + readFilesNames[imate][ifile] + " > "+ outFileTmp+"/readFilesIn.info 2>&1").c_str());
                ifstream readFilesIn_info((outFileTmp+"/readFilesIn.info").c_str());
                inOut->logMain <<readFilesIn_info.rdbuf();

                readsCommandFile <<"echo FILE "<< ifile << "\n";
                readsCommandFile << readFilesCommandString <<"   "<< ("\""+readFilesNames[imate][ifile]+"\"") <<"\n";
            };

            readsCommandFile.flush();
            readsCommandFile.seekg(0,ios::beg);
            inOut->logMain <<"\n   readsCommandsFile:\n"<<readsCommandFile.rdbuf()<<endl;
            readsCommandFile.close();

            chmod(readsCommandFileName.at(imate).c_str(),S_IXUSR | S_IRUSR | S_IWUSR);

            readFilesCommandPID[imate]=0;

            ostringstream errOut;
            pid_t PID=vfork();
            switch (PID) {
                case -1:
                    errOut << "EXITING: because of fatal EXECUTION error: Failed vforking readFilesCommand\n";
                    errOut << errno << ": " << strerror(errno) << "\n";
                    exitWithError(errOut.str(), std::cerr, inOut->logMain, EXIT_CODE_PARAMETER, *this);
                    break;

                case 0:
                    //this is the child
                    execlp(readsCommandFileName.at(imate).c_str(), readsCommandFileName.at(imate).c_str(), (char*) NULL);
                    exit(0);

                default:
                    //this is the father, record PID of the children
                    readFilesCommandPID[imate]=PID;
            };

            inOut->readIn[imate].open(readFilesInTmp.at(imate).c_str());
        };

    };
    readFilesIndex=0;

    if (readFilesTypeN==10) {//SAM file - skip header lines
        readSAMheader(readFilesCommandString, readFilesNames.at(0));
    };
 
};
