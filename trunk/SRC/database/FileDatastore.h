/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision: 1.11 $
// $Date: 2005-11-07 23:53:45 $
// $Source: /usr/local/cvs/OpenSees/SRC/database/FileDatastore.h,v $
                                                                        
                                                                        
#ifndef FileDatastore_h
#define FileDatastore_h

// Written: fmk 
// Created: 10/98
//
// Description: This file contains the class definition for FileDatastore.
// FileDatastore is a concrete subclas of FE_Datastore. A FileDatastore 
// object is used in the program to store/restore the geometry and state 
// information in a domain at a particular instance in the analysis. The
// information is stored in text files.
//
// What: "@(#) FileDatastore.h, revA"

#include <FE_Datastore.h>

#include <fstream>
using std::fstream;

class FEM_ObjectBroker;

#define maxIDsize   1024
#define maxVectSize 1024
#define maxMatSize  512

struct IDdata{
  int dbTag;
  int commitTag;
  int data[maxIDsize];     
};

struct VectData{
  int dbTag;
  int commitTag;
  double data[maxVectSize];     
};

struct MatrixData{
  int dbTag;
  int commitTag;
  double data[maxMatSize];     
};

struct FileEnds{
  long int ids[maxIDsize];
  long int vects[maxVectSize];
  long int mats[maxMatSize];
};

struct CurrentFilePos{
  long int ids[maxIDsize];
  long int vects[maxVectSize];
  long int mats[maxMatSize];
};

struct CurrentFileCommitTag{
  int ids[maxIDsize];
  int vects[maxVectSize];
  int mats[maxMatSize];
};

struct MaxFileDbTag{
  int ids[maxIDsize];
  int vects[maxVectSize];
  int mats[maxMatSize];
};


class FileDatastore: public FE_Datastore
{
  public:
    FileDatastore(const char *dataBase,
		  Domain &theDomain, 
		  FEM_ObjectBroker &theBroker);    
    
    ~FileDatastore();

    // methods for sending and receiving the data
    int sendMsg(int dbTag, int commitTag, 
		const Message &, 
		ChannelAddress *theAddress =0);    
    int recvMsg(int dbTag, int commitTag, 
		Message &, 
		ChannelAddress *theAddress =0);        

    int sendMatrix(int dbTag, int commitTag, 
		   const Matrix &theMatrix, 
		   ChannelAddress *theAddress =0);
    int recvMatrix(int dbTag, int commitTag, 
		   Matrix &theMatrix, 
		   ChannelAddress *theAddress =0);
    
    int sendVector(int dbTag, int commitTag, 
		   const Vector &theVector, 
		   ChannelAddress *theAddress =0);
    int recvVector(int dbTag, int commitTag, 
		   Vector &theVector, 
		   ChannelAddress *theAddress =0);
    
    int sendID(int dbTag, int commitTag,
	       const ID &theID,
	       ChannelAddress *theAddress =0);
    int recvID(int dbTag, int commitTag,
	       ID &theID,
	       ChannelAddress *theAddress =0);

    int createTable(const char *tableName, int numColumns, char *columns[]);
    int insertData(const char *tableName, char *columns[], 
		   int commitTag, const Vector &data);
    int getData(const char *tableName, char *columns[], int commitTag, Vector &data);

    // the commitState method
    int commitState(int commitTag);        
    
  protected:

  private:
    void resetFilePointers(void);

    // Private methods
    fstream *openFile(char *fileName);
    
    // private attributes
    char *dataBase;
    fstream **ids, **vects, **mats;
    int lastDomainChangeStamp;

    struct MatrixData matBuffer;    
    struct VectData vectBuffer;
    struct IDdata idBuffer;
    struct FileEnds fileEnds;

    struct CurrentFilePos filePos;
    struct CurrentFileCommitTag fileCommitTags;
    struct MaxFileDbTag fileDbTags;    
    
    int currentCommitTag;
};




#endif

