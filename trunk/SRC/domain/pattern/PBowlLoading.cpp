//===============================================================================
//# COPYRIGHT (C): Woody's license (by BJ):
//                 ``This    source  code is Copyrighted in
//                 U.S.,  for  an  indefinite  period,  and anybody
//                 caught  using it without our permission, will be
//                 mighty good friends of ourn, cause we don't give
//                 a  darn.  Hack it. Compile it. Debug it. Run it.
//                 Yodel  it.  Enjoy it. We wrote it, that's all we
//                 wanted to do.''
//
//# PROJECT:           Object Oriented Finite Element Program
//# PURPOSE:           Plastic Bowl (aka Domain Reduction) implementation:
//#                    This file contains the class definition
//#                    for PBowlLoading.
//#                    PBowlLoading is an subclass of loadPattern.
//# CLASS:             PBowlLoading
//#
//# VERSION:           0.61803398874989 (golden section)
//# LANGUAGE:          C++
//# TARGET OS:         all...
//# DESIGN:            Zhaohui Yang, Boris Jeremic
//# PROGRAMMER(S):     Jinxou Liao, Zhaohui Yang, Boris Jeremic
//#
//#
//# DATE:              21Oct2002
//# UPDATE HISTORY:    31Oct2002 fixed some memory leaks
//#                    04Nov2002 changed the way plastic bowl elements are
//#                     input.
//#                    10Nov2002 Zeroing diagonal and out of diagaonal blocks
//#                     for b<->e nodes
//#                    13Nov2002 changes to split "b" and "e" nodes within
//#                     the plastic bowl elements. Also, simple definition of
//#                     of cubic bowl is now facilitated ...
//#
//#
//#
//===============================================================================

// Purpose: This file contains the class definition for PBowlLoading.
// PBowlLoading is an subclass of loadPattern.

#include <PBowlLoading.h>



PBowlLoading::PBowlLoading()
:LoadPattern(0, PATTERN_TAG_PBowlLoading),
PBowlElements(0),
ExteriorNodes(0),
BoundaryNodes(0),
PBowlLoads(0),
U(0),
Udd(0)
{

}

PBowlLoading::PBowlLoading(int tag,
                           char *PBEfName,
                           char *DispfName,
                           char *AccefName,
                           double theTimeIncr,
                           double theFactor,
                           double xplus,
                           double xminus,
                           double yplus,
                           double yminus,
                           double zplus,
                           double zminus)
:LoadPattern(tag,
             PATTERN_TAG_PBowlLoading),
             PBTimeIncr(theTimeIncr),
             cFactor(theFactor),
	     xPlus(xplus),
	     xMinus(xminus),
	     yPlus(yplus),
	     yMinus(yminus),
	     zPlus(zplus),
	     zMinus(zminus)
  {
    // determine the number of data points .. open file and count num entries
    int timeSteps1, timeSteps2;
    int numDataPoints =0;
    double dataPoint;
    int eleID;


    ifstream theFile;

  //--------------------------------
  //Input displacement data
  //--------------------------------
    theFile.open(DispfName);
    if (theFile.bad())
      {
        cerr << "WARNING - PBowlLoading::PBowlLoading()";
        cerr << " - could not open file " << DispfName << endl;
        exit(1);
      }
    else
      {
        //Input the number of time steps first
        theFile >> timeSteps1;
        //Loop to count the number of data points
        while (theFile >> dataPoint)
          numDataPoints++;
      }
    theFile.close();
    UnumDataPoints = numDataPoints;
    thetimeSteps = timeSteps1;
    if ( timeSteps1 == 0)
      {
        cerr << "WARNING - PBowlLoading::PBowlLoading()";
        cerr << " - Time steps for displacement equal to zero... " << AccefName << endl;
        exit(0);
      }

  // create a vector and read in the data
    if (numDataPoints != 0) {

    // first open the file
    theFile.open(DispfName, ios::in);
    if (theFile.bad()) {
      cerr << "WARNING - PBowlLoading::PBowlLoading()";
      cerr << " - could not open file " << DispfName << endl;
     } else {

      // now create the vector
      if ( numDataPoints - thetimeSteps*(numDataPoints/thetimeSteps) != 0 ) {
         cerr << "WARNING - PBowlLoading::PBowlLoading()";
         cerr << " - Input data not sufficient...Patched with zeros " << DispfName << endl;
      }
      int cols = numDataPoints/thetimeSteps;
      U = new Matrix(cols, thetimeSteps);

      // ensure we did not run out of memory
      if (U == 0 || U->noRows() == 0 || U->noCols() == 0) {
         cerr << "PBowlLoading::PBowlLoading() - ran out of memory constructing";
         cerr << " a Matrix of size (cols*rows): " << cols << " * " << thetimeSteps << endl;

      if (U != 0)
         delete U;
         U = 0;
      }

      // read the data from the file
      else {
         theFile >> timeSteps1;
         for (int t=0; t< thetimeSteps; t++)
           for  (int j=0;j<cols; j++) {
               theFile >> dataPoint;
               (*U)(j, t) = dataPoint;
           }
      }
     }

      // finally close the file
      theFile.close();
  }

  //--------------------------------
  //Input acceleration data
  //--------------------------------
  theFile.open(AccefName);
  numDataPoints = 0;

  if (theFile.bad()) {
    cerr << "WARNING - PBowlLoading::PBowlLoading()";
    cerr << " - could not open file " << AccefName << endl;
    exit(2);
  } else {
    //Input the number of time steps first
    theFile >> timeSteps2;
    //Loop to count the number of data points
    while (theFile >> dataPoint)
      numDataPoints++;
  }
  theFile.close();
  UddnumDataPoints = numDataPoints;
  if ( timeSteps1 !=  timeSteps2) {
    cerr << "WARNING - PBowlLoading::PBowlLoading()";
    cerr << " - Time steps for displacement not equal to that for acceleration... " << AccefName << endl;
  }


  // create a vector and read in the data
  if (numDataPoints != 0) {

    // first open the file
    theFile.open(AccefName, ios::in);
    if (theFile.bad()) {
      cerr << "WARNING - PBowlLoading::PBowlLoading()";
      cerr << " - could not open file " << AccefName << endl;
    } else {

      // now create the vector
      if ( numDataPoints - thetimeSteps*(numDataPoints/thetimeSteps) != 0 ) {
         cerr << "WARNING - PBowlLoading::PBowlLoading()";
         cerr << " - Input data not sufficient...Patched with zeros " << AccefName << endl;
      }
      int cols = numDataPoints/thetimeSteps;
      Udd = new Matrix(cols, thetimeSteps);

      // ensure we did not run out of memory
      if (Udd == 0 || Udd->noRows() == 0 || Udd->noCols() == 0) {
         cerr << "PBowlLoading::PBowlLoading() - ran out of memory constructing";
         cerr << " a Matrix of size (cols*rows): " << cols << " * " << thetimeSteps << endl;

      if (Udd != 0)
        delete Udd;
        Udd = 0;
      }

      // read the data from the file
      else {
        theFile >> timeSteps2;
        for (int t=0; t< thetimeSteps; t++)
           for  (int j=0;j<cols; j++) {
              theFile >> dataPoint;
              (*Udd)(j, t) = dataPoint;
           }
        }
    }

    // finally close the file
    theFile.close();
  }


  //--------------------------------
  //Adding plastic bowl elements
  //from file to PBowlElements
  //--------------------------------
  theFile.open(PBEfName);
  numDataPoints = 0;
  int numPBE = 0;

  if (theFile.bad()) {
    cerr << "WARNING - PBowlLoading::PBowlLoading()";
    cerr << " - could not open file " << PBEfName << endl;
    exit(2);
  } else {
    //Input the number of Plastic Bowl elements
    theFile >> numPBE;
    //Loop to count the number of data points
    while (theFile >> eleID)
      numDataPoints++;
  }
  theFile.close();
  if ( numPBE !=  numDataPoints) {
    cerr << "WARNING - PBowlLoading::PBowlLoading()";
    cerr << " - Number of plastic bowl elements not equal to the number of elements provided... " << numDataPoints << numPBE << PBEfName << endl;
    exit(3);
  }


  // create a vector and read in the data
  if (numDataPoints != 0) {

    // first open the file
    theFile.open(PBEfName, ios::in);
    if (theFile.bad()) {
      cerr << "WARNING - PBowlLoading::PBowlLoading()";
      cerr << " - could not open file " << AccefName << endl;
    } else {

      // now create the vector
      PBowlElements = new ID(numPBE);

      // ensure we did not run out of memory
      if (PBowlElements == 0 || PBowlElements->Size() == 0 ) {
         cerr << "PBowlLoading::PBowlLoading() - ran out of memory constructing";
         cerr << " a ID of size: " << PBowlElements->Size()<< endl;

         if (PBowlElements != 0)
          delete PBowlElements;
         PBowlElements = 0;
      }

      // read the data from the file
      else {
        theFile >> numPBE;
        int i;
	for (i=0; i< numPBE; i++) {
              theFile >> eleID;
              (*PBowlElements)(i) = eleID;
        }
      }

      // finally close the file
      theFile.close();

      //Check if read in correctly
//test      cout << "# of plastic bowl element: " << numPBE << endl;
//test      cout <<  (*PBowlElements);
    }
  }

  LoadComputed = false;
}



PBowlLoading::~PBowlLoading()
{
  // invoke the destructor on all ground motions supplied

  if ( PBowlElements != 0)
    delete PBowlElements;

  if ( ExteriorNodes != 0)
    delete ExteriorNodes;

  if ( BoundaryNodes != 0)
    delete BoundaryNodes;

  if ( PBowlLoads != 0)
    delete PBowlLoads;

  if ( U != 0)
    delete U;

  if ( Udd != 0)
    delete Udd;

}


void
PBowlLoading::setDomain(Domain *theDomain)
{
  this->LoadPattern::setDomain(theDomain);

  // // now we go through and set all the node velocities to be vel0
  // if (vel0 != 0.0) {
  //   NodeIter &theNodes = theDomain->getNodes();
  //   Node *theNode;
  //   Vector newVel(1);
  //   int currentSize = 1;
  //   while ((theNode = theNodes()) != 0) {
  //     int numDOF = theNode->getNumberDOF();
  //     if (numDOF != currentSize)
  //   newVel.resize(numDOF);
  //
  //     newVel = theNode->getVel();
  //     newVel(theDof) = vel0;
  //     theNode->setTrialVel(newVel);
  //     theNode->commitState();
  //   }
  // }
}


void
PBowlLoading::applyLoad(double time)
{

  Domain *theDomain = this->getDomain();
  if (theDomain == 0)
    return;

  //Finding the all the nodes in the plastic bowl and sort it ascendingly
  if ( !LoadComputed )
     this->CompPBLoads();

  // see if quick return, i.e. no plastic bowl nodes or domain set
  int numBnodes = BoundaryNodes->Size();
  int numEnodes = ExteriorNodes->Size();
  if ( (numBnodes + numEnodes) == 0)
    return;

  //Apply loads on each boundary bowl nodes
  Node *theNode;
  for (int i=0; i<numBnodes; i++) {
    Vector load=this->getNodalLoad((*BoundaryNodes)[i], time);
    //Take care of the minus sign in the effective seismic force for boundary nodes
    load = load*(-1.0);
    theNode = theDomain->getNode( (*BoundaryNodes)[i] );
    theNode->addUnbalancedLoad(load);
  }

  //Apply loads on each exterior bowl nodes
  for (int i=0; i<numEnodes; i++) {
    const Vector &load=this->getNodalLoad((*ExteriorNodes)[i], time);
    theNode = theDomain->getNode( (*ExteriorNodes)[i] );
    theNode->addUnbalancedLoad(load);
  }
}

int
PBowlLoading::sendSelf(int commitTag, Channel &theChannel)
{
  cerr << "PBowlLoading::sendSelf() - not yet implemented\n";
  return 0;
}

int
PBowlLoading::recvSelf(int commitTag, Channel &theChannel,
       FEM_ObjectBroker &theBroker)
{
  cerr << "PBowlLoading::recvSelf() - not yet implemented\n";
  return 0;
}

/* **********************************************************************************************
int
PBowlLoading::sendSelf(int commitTag, Channel &theChannel)
{
  // first send the tag and info about the number of ground motions
  int myDbTag = this->getDbTag();
  ID theData(2);
  theData(0) = this->getTag();
  theData(1) = numMotions;

  if (theChannel.sendID(myDbTag, commitTag, theData) < 0) {
    g3ErrorHandler->warning("PBowlLoading::sendSelf - channel failed to send the initial ID");
    return -1;
  }

  // now for each motion we send it's classsss tag and dbtag
  ID theMotionsData(2*numMotions);
  for (int i=0; i<numMotions; i++) {
    theMotionsData[i] = theMotions[i]->getClassTag();
    int motionsDbTag = theMotions[i]->getDbTag();
    if (motionsDbTag == 0) {
      motionsDbTag = theChannel.getDbTag();
      theMotions[i]->setDbTag(motionsDbTag);
    }
    theMotionsData[i+numMotions] = motionsDbTag;
  }

  if (theChannel.sendID(myDbTag, commitTag, theMotionsData) < 0) {
    g3ErrorHandler->warning("PBowlLoading::sendSelf - channel failed to send the motions ID");
    return -1;
  }

  // now we send each motion
  for (int j=0; j<numMotions; j++)
    if (theMotions[j]->sendSelf(commitTag, theChannel) < 0) {
      g3ErrorHandler->warning("PBowlLoading::sendSelf - motion no: %d failed in sendSelf", j);
      return -1;
    }

  // if get here successfull
  return 0;
}

int
PBowlLoading::recvSelf(int commitTag, Channel &theChannel,
       FEM_ObjectBroker &theBroker)
{
  // first get the tag and info about the number of ground motions from the Channel
  int myDbTag = this->getDbTag();
  ID theData(2);
  if (theChannel.recvID(myDbTag, commitTag, theData) < 0) {
    g3ErrorHandler->warning("PBowlLoading::recvSelf - channel failed to recv the initial ID");
    return -1;
  }

  // set current tag
  this->setTag(theData(0));

  // now get info about each channel
  ID theMotionsData (2*theData(1));
  if (theChannel.recvID(myDbTag, commitTag, theMotionsData) < 0) {
    g3ErrorHandler->warning("PBowlLoading::recvSelf - channel failed to recv the motions ID");
    return -1;
  }


  if (numMotions != theData(1)) {

    //
    // we must delete the old motions and create new ones and then invoke recvSelf on these new ones
    //

    if (numMotions != 0) {
      for (int i=0; i<numMotions; i++)
  delete theMotions[i];
      delete [] theMotions;
    }
    numMotions = theData[1];
    theMotions = new (GroundMotion *)[numMotions];
    if (theMotions == 0) {
      g3ErrorHandler->warning("PBowlLoading::recvSelf - out of memory creating motion array of size %d\n", numMotions);
      numMotions = 0;
      return -1;
    }

    for (int i=0; i<numMotions; i++) {
      theMotions[i] = theBroker.getNewGroundMotion(theMotionsData[i]);
      if (theMotions[i] == 0) {
  g3ErrorHandler->warning("PBowlLoading::recvSelf - out of memory creating motion array of size %d\n", numMotions);
  numMotions = 0;
  return -1;
      }
      theMotions[i]->setDbTag(theMotionsData[i+numMotions]);
      if (theMotions[i]->recvSelf(commitTag, theChannel, theBroker) < 0) {
  g3ErrorHandler->warning("PBowlLoading::recvSelf - motion no: %d failed in recvSelf", i);
  numMotions = 0;
  return -1;
      }
    }

  } else {

    //
    // we invoke rrecvSelf on the motions, note if a motion not of correct class
    // we must invoke the destructor on the motion and create a new one of correct type
    //

    for (int i=0; i<numMotions; i++) {
      if (theMotions[i]->getClassTag() == theMotionsData[i]) {
  if (theMotions[i]->recvSelf(commitTag, theChannel, theBroker) < 0) {
    g3ErrorHandler->warning("PBowlLoading::recvSelf - motion no: %d failed in recvSelf", i);
    return -1;
  }
      } else {
  // motion not of correct type
  delete theMotions[i];
  theMotions[i] = theBroker.getNewGroundMotion(theMotionsData[i]);
  if (theMotions[i] == 0) {
    g3ErrorHandler->warning("PBowlLoading::recvSelf - out of memory creating motion array of size %d\n", numMotions);
    numMotions = 0;
    return -1;
  }
  theMotions[i]->setDbTag(theMotionsData[i+numMotions]);
  if (theMotions[i]->recvSelf(commitTag, theChannel, theBroker) < 0) {
    g3ErrorHandler->warning("PBowlLoading::recvSelf - motion no: %d failed in recvSelf", i);
    numMotions = 0;
    return -1;
  }
      }
    }
  }

  // if get here successfull
  return 0;
}


***************************************************************************************** */

void
PBowlLoading::Print(ostream &s, int flag)
{
  cerr << "PBowlLoading::Print() - not yet implemented\n";
}



// method to obtain a blank copy of the LoadPattern
LoadPattern *
PBowlLoading::getCopy(void)
{

  cerr << "PBowlLoading::getCopy() - not yet implemented\n";
  return 0;
}

/* ****************************************************************************************
// method to obtain a blank copy of the LoadPattern
LoadPattern *
PBowlLoading::getCopy(void)
{
  PBowlLoading *theCopy = new PBowlLoading(0, 0);
  theCopy->setTag(this->getTag);
  theCopy->numMotions = numMotions;
  theCopy->theMotions = new (GroundMotion *)[numMotions];
  for (int i=0; i<numMotions; i++)
    theCopy->theMotions[i] = theMotions[i];

  return 0;
}
***************************************************************************************** */

// void
// PBowlLoading::addPBElements(const ID &PBEle)
// {
//   // create a copy of the vector containg plastic bowl elements
//   PBowlElements = new ID(PBEle);
//   // ensure we did not run out of memory
//   if (PBowlElements == 0 || PBowlElements->Size() == 0) {
//     cerr << "PBowlLoading::addPBElements() - ran out of memory constructing";
//     cerr << " a Vector of size: " <<  PBowlElements->Size() << endl;
//     if (PBowlElements != 0)
//       delete PBowlElements;
//     PBowlElements = 0;
//   }
//
// }


void
PBowlLoading::CompPBLoads()
{
  //===========================================================
  // Finding all plastic bowl nodes, Joey Yang Oct. 18, 2002
  //===========================================================
  Domain *theDomain = this->getDomain();

  //Assume all the plastic bowl elements have the same number of nodes
  Element *theElement = theDomain->getElement( (*PBowlElements)(0) );
  int NIE = theElement->getNumExternalNodes();

  int max_bnode = PBowlElements->Size() * NIE;
  ID *Bowl_node = new ID(max_bnode);
  ID *Bound_node = new ID(max_bnode);
  ID NidesinFirstEle = theElement->getExternalNodes();

  int i, j, k;
  //Inital node list from the first plastic bowl element
  for (i = 0; i<NIE; i++)
     (*Bowl_node)(i) = NidesinFirstEle(i);

  int no_bnode = NIE;
  int no_boundarynodes = 0, no_exteriornodes = 0;
  int Bowl_elem_nb = PBowlElements->Size();
  ID Temp;

  for ( i=1; i<Bowl_elem_nb; i++)
  {
      theElement = theDomain->getElement( (*PBowlElements)(i) );
      Temp = theElement->getExternalNodes();
      for ( j=0;j<NIE;j++)
      {
        for (k = 0; k< no_bnode; k++) {
           if ( Temp(j) == (*Bowl_node)(k) )
           //cout << Temp(j) << "  " << Bowl_node(k) << endl;
           break;
        } //endofIF

        //If no duplicate, add new node; otherwise, skip
        if ( k == no_bnode) {
          (*Bowl_node)(no_bnode) = Temp(j);
          no_bnode ++;
        } //end of for (k=0...)

      }//end of for (j=0...)

  }
  //--Joey------------------------------------------------

  //test //check the bowl nodes
  //test cout << "\nCheck all plastic bowl nodes...\n";
  //test for (int bi=0;bi<no_bnode;bi++)
  //test    cout<< (*Bowl_node)(bi) <<"  ";
  //test cout<< endl << "# of pbowl nodes = " << no_bnode<<endl;
  //test //cout<<"finish inputting  and organizing the Bowl_node array"<<endl;


  //Find all nodes on the boundary Joey Yang & Boris Jeremic 11/06/02
  if ( (BoundaryNodes == 0) && ( (xPlus == xMinus) || (yPlus == yMinus) || (zPlus == zMinus) ) ) {
    cerr << "PBowlLoading::CompPBLoads() - Boundary node specification not correct..." << endl;
    exit(1);
  }

  no_boundarynodes = 0;
  for (i =0; i < no_bnode; i++) {
    Node *theNode = theDomain->getNode( (*Bowl_node)(i) );
    const Vector &coor = theNode->getCrds();

    //right face
    if ( (coor(0) == xPlus) && (coor(1) >= yMinus) && (coor(1) <= yPlus) && (coor(2) >= zMinus) && (coor(2) <= zPlus) ) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
    //left face
    else if ( (coor(0) == xMinus) && (coor(1) >= yMinus) && (coor(1) <= yPlus) && (coor(2) >= zMinus) && (coor(2) <= zPlus)) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
    //upper face
    else if ( (coor(1) == yPlus) && (coor(0) >= xMinus) && (coor(0) <= xPlus) && (coor(2) >= zMinus) && (coor(2) <= zPlus)) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
    //lower face
    else if ( (coor(1) == yMinus) && (coor(0) >= xMinus) && (coor(0) <= xPlus) && (coor(2) >= zMinus) && (coor(2) <= zPlus)) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
    //top face
    else if ( (coor(2) == zPlus) && (coor(0) >= xMinus) && (coor(0) <= xPlus) && (coor(1) >= yMinus) && (coor(1) <= yPlus)) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
    //bottom face
    else if ( (coor(2) == zMinus) && (coor(0) >= xMinus) && (coor(0) <= xPlus) && (coor(1) >= yMinus) && (coor(1) <= yPlus)) {
       (*Bound_node)(no_boundarynodes) = (*Bowl_node)(i);
       no_boundarynodes++;
    }
  }

  //Adding all boundary nodes on the plastic bowl
  BoundaryNodes = new ID(no_boundarynodes);

  if (BoundaryNodes == 0 || BoundaryNodes->Size() == 0) {
    cerr << "PBowlLoading::CompPBLoads() - ran out of memory constructing";
    cerr << " a Vector of size: " <<  BoundaryNodes->Size() << endl;
    if (BoundaryNodes != 0)
      delete BoundaryNodes;
    BoundaryNodes = 0;
  }

  no_exteriornodes = no_bnode;
  for (i =0; i < no_boundarynodes; i++) {
    (*BoundaryNodes)(i) = (*Bound_node)(i);

    //Sift out all the boundary nodes from Bowl_node
    for (j = 0; j < no_bnode; j++) {
       if ( (*Bowl_node)(j) == (*Bound_node)(i) ) {
          (*Bowl_node)(j) = 0; //zero this boundary nodes
	  no_exteriornodes --;
       }
    }//end of sifting loop
  }//end of adding boundary node loop

  //Adding all exterior nodes on the plastic bowl
  ExteriorNodes = new ID(no_exteriornodes);
  if (ExteriorNodes == 0 || ExteriorNodes->Size() == 0) {
    cerr << "PBowlLoading::CompPBLoads() - ran out of memory constructing";
    cerr << " a Vector of size: " <<  ExteriorNodes->Size() << endl;
    if (ExteriorNodes != 0)
      delete ExteriorNodes;
    ExteriorNodes = 0;
  }

  j = 0;
  for (i =0; i < no_bnode; i++) {
    if ( (*Bowl_node)(i) != 0 ) {
       (*ExteriorNodes)(j) = (*Bowl_node)(i);
       j++;
    }
  }

  //test //check the boundary bowl nodes
  //test cout << "\nCheck all boundary bowl nodes...\n";
  //test for (int bi = 0; bi < no_boundarynodes; bi++)
  //test      cout << (*BoundaryNodes)(bi) <<"  ";
  //test cout<< endl << "# of boundary bowl nodes = " << no_boundarynodes << endl;
  //test
  //test //check the exterior bowl nodes
  //test cout << "\nCheck all exterior bowl nodes...\n";
  //test for (int bi = 0; bi < no_exteriornodes; bi++)
  //test      cout << (*ExteriorNodes)(bi) <<"  ";
  //test cout<< endl << "# of exterior bowl nodes = " << no_exteriornodes << endl;
  //test
  //test cout<<"finish inputting  and organizing the Bowl_node array"<<endl;


  //===========================================================
  // Computing the equivalent(effective) forces for all plastic bowl nodes
  //===========================================================

  int cols = Udd->noRows();
  //Matrix to hold the computed effective nodal forces for all plastic bowl DOFs and each time step
  Matrix *F = new Matrix(cols, thetimeSteps);

  //Assume all plastic bowl nodes have the same number of DOFs
  Node *theNode = theDomain->getNode((*BoundaryNodes)(0));
  int NDOF = theNode->getNumberDOF();

  Vector *Fm = new Vector(NIE*NDOF);
  Vector *Fk  = new Vector(NIE*NDOF);
  //Matrix *Ke= new Matrix(NIE*NDOF,NIE*NDOF);
  //Matrix *Me= new Matrix(NIE*NDOF,NIE*NDOF);
  Vector *u_e = new Vector(NIE*NDOF);
  Vector *udd_e = new Vector(NIE*NDOF);


  // intialize the F()
  for ( i=0;i<cols; i++)
      for ( j=0;j<thetimeSteps; j++)
         (*F)(i,j)=0;

  Element *theBowlElements;

  for ( i=0; i<Bowl_elem_nb; i++)
  {
   // get the Brick;
   theBowlElements = theDomain->getElement( (*PBowlElements)(i) );
   const ID &nd = theBowlElements->getExternalNodes();

   Matrix Me = theBowlElements ->getMass();
   Matrix Ke = theBowlElements ->getTangentStiff();

   //-------------------------------------------------------------------------
   //Zero diagonal block entries of boundary and exterior nodes, respectively
   //-------------------------------------------------------------------------

   //Find out the boundary and exterior nodes in this element
   ID *B_node = new ID(NIE);  //array to store the indices of all boundary nodes
   ID *E_node = new ID(NIE);  //array to store the indices of all exterior nodes
   int nB=0, nE=0;
   bool bdnode;

   for ( int ii = 0; ii < NIE; ii++) {
    //Check if nd(ii) is boundary node or not
    bdnode = false;
    for (int id = 0; id < no_boundarynodes; id++) {
       if ( nd(ii) == (*BoundaryNodes)(id) ) {
         bdnode = true;
       	 break;
       }
    }

    if ( bdnode ) {
       (*B_node)(nB) = ii;
       nB ++;
    }
    else {
       (*E_node)(nE) = ii;
       nE ++;
    }
   }

   //test cout << "B nodes: " << nB << " " << *B_node;
   //test cout << "E nodes: " << nE << " " << *E_node;

   //cout << " ...Me before  " << Me;
   //Zero out the diagonal block of Boundary nodes
   for (int m = 0; m < nB; m++)
     for (int n = 0; n < nB; n++)
      for (int d = 0; d < NDOF; d++)
        for (int e= 0; e < NDOF; e++) {
          Me( (*B_node)(m)*NDOF+d, (*B_node)(n)*NDOF+e ) = 0.0;
          Ke( (*B_node)(m)*NDOF+d, (*B_node)(n)*NDOF+e ) = 0.0;
        }

   //Zero out the diagonal block of Exterior nodes
   for (int m = 0; m < nE; m++)
     for (int n = 0; n < nE; n++)
      for (int d = 0; d < NDOF; d++)
        for (int e= 0; e < NDOF; e++) {
          Me( (*E_node)(m)*NDOF+d, (*E_node)(n)*NDOF+e ) = 0.0;
          Ke( (*E_node)(m)*NDOF+d, (*E_node)(n)*NDOF+e ) = 0.0;
        }

   //test cout << " ...Me after  " << Me;
   //test cout << " ...Ke after  " << Ke;

   //   get the u and u_dotdot for this element
   for ( int t=0;t<thetimeSteps; t++)
   {
     //cout << "element: " << i << "" << " Time step: " << t << endl;
     for (int j=0;j<NIE;j++)  //BJ make it into # of nodes per element (2D or 3D...)
     {
       for (int d=0;d<NDOF;d++)
       {
           (*u_e)(j*NDOF+d)    =   (*U)( nd(j)*NDOF-NDOF+d,t);
         (*udd_e)(j*NDOF+d)    = (*Udd)( nd(j)*NDOF-NDOF+d,t);
       }
     }

     Fm->addMatrixVector(0.0, Me, (*udd_e), 1.0);
     Fk->addMatrixVector(0.0, Ke, (*u_e), 1.0);

     for (int k=0;k<NIE; k++)
        for (int d=0;d<NDOF;d++) {
            (*F)( nd(k)*NDOF-NDOF+d,t) = (*F)( nd(k)*NDOF-NDOF+d,t) + (*Fk)(k*NDOF+d) + (*Fm)(k*NDOF+d);
	}

   } //end for timestep

  }  // end for bowl element

  PBowlLoads = new Matrix(*F);

  // ensure we did not run out of memory
  if (PBowlLoads->noRows() == 0 || PBowlLoads->noCols() == 0 ) {
    cerr << "PBowlLoading::PBowlLoads() - ran out of memory";
    cerr << " a Matrix of size: " <<  PBowlLoads->noRows() << " * " << PBowlLoads->noCols() << endl;
  }

//test  cout<<"\nFinish calculating the forces..." << endl << endl;
  LoadComputed = true;

  delete Fm;
  delete Fk;
  delete u_e;
  delete udd_e;
  delete F;

}


const Vector &
PBowlLoading::getNodalLoad(int nodeTag, double time)
{
  Vector *dummy = new Vector(0);
  //Get the node
  Domain *theDomain = this->getDomain();
  Node *theNode = theDomain->getNode(nodeTag);
  if (theNode == 0) {
     cerr << "PBowlLoading::getNodalLoad() - no nodes associtated to the nodeTag " << nodeTag << "\n";
     return ( *dummy );
  }

  delete dummy;

  //Create the nodal load vector accoding to the DOFs the node has
  int numDOF = theNode->getNumberDOF();
  Vector *nodalLoad = new Vector(numDOF);


  //Get the nodal loads associated to the nodeTag
  // check for a quick return
  if (time < 0.0 || PBowlLoads == 0)
    return (*nodalLoad);

  // determine indexes into the data array whose boundary holds the time
  double incr = time/PBTimeIncr;
  int incr1 = (int) floor(incr)-1;
  int incr2 = incr1 + 1;
  double value1=0, value2=0;

  int i;
  if ( incr2 == thetimeSteps ) {
    for (i = 0; i < numDOF; i++)
       (*nodalLoad)(i) = (*PBowlLoads)(i, incr1);
  }

  //If beyond time step, return 0 loads
  else if (incr2 > thetimeSteps ) {
      //test if ( nodeTag == 109)
      //test    cout << "Time = " << time << " Node # " << nodeTag  << " " << (*nodalLoad) << endl;
      return (*nodalLoad);
  }

  //If within time step, return interpolated values
  else {
    for (i = 0; i < numDOF; i++){
       value1 = (*PBowlLoads)(numDOF*(nodeTag-1)+i, incr1);
       value2 = (*PBowlLoads)(numDOF*(nodeTag-1)+i, incr2);
       (*nodalLoad)(i) = cFactor*(value1 + (value2-value1)*(time/PBTimeIncr - incr2));
    }
  }

  //test if ( nodeTag == 109) {
  //test    cout << "Time = " << time << " Node # " << nodeTag  << " " << (*nodalLoad) << endl;
  //test }

  return (*nodalLoad);

}
