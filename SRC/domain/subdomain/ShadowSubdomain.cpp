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
                                                                        
// $Revision: 1.3 $
// $Date: 2003-08-29 07:47:20 $
// $Source: /usr/local/cvs/OpenSees/SRC/domain/subdomain/ShadowSubdomain.cpp,v $
                                                                        
// Written: fmk 
// Revision: A
//
// Description: This file contains the class definition for ShadowSubdomain.
// ShadowSubdomain is a container class. The class is responsible for holding
// and providing access to the Elements, Nodes, LoadCases, SP_Constraints 
// and MP_Constraints that have been added to the ShadowSubdomain.
//
// What: "@(#) ShadowSubdomain.C, revA"


#include <ShadowSubdomain.h>
#include <stdlib.h>

#include <Node.h>
#include <Element.h>
#include <SP_Constraint.h>
#include <MP_Constraint.h>
#include <DomainDecompositionAnalysis.h>
#include <NodalLoad.h>
#include <ElementalLoad.h>
#include <FEM_ObjectBroker.h>
#include <Timer.h>
#include <LoadPattern.h>

#include <ArrayOfTaggedObjects.h>
#include <SingleDomNodIter.h>
#include <SingleDomEleIter.h>
#include <Graph.h>
#include <FE_Element.h>
#include <PartitionedModelBuilder.h>

#include <EquiSolnAlgo.h>
#include <IncrementalIntegrator.h>
#include <LinearSOE.h>

#include <FE_Element.h>

#include <ShadowActorSubdomain.h>

int ShadowSubdomain::count = 0; // MHS
int ShadowSubdomain::numShadowSubdomains = 0;
ShadowSubdomain **ShadowSubdomain::theShadowSubdomains = 0;


ShadowSubdomain::ShadowSubdomain(int tag, 
				 MachineBroker &theMachineBroker,
				 FEM_ObjectBroker &theObjectBroker)
  :Shadow(ACTOR_TAGS_SUBDOMAIN, theObjectBroker, theMachineBroker, 0),
	  
   Subdomain(tag),
   msgData(4),
   theElements(0,128),
   theNodes(0,128),
   theExternalNodes(0,128), 
   theLoadCases(0,128),
   numDOF(0),numElements(0),numNodes(0),numExternalNodes(0),
   numSPs(0),numMPs(0), buildRemote(false), gotRemoteData(false), 
   theFEele(0),
   theVector(0), theMatrix(0)

{
  
  numShadowSubdomains++;

  ShadowSubdomain **theCopy = new ShadowSubdomain *[numShadowSubdomains];

  for (int i = 0; i < numShadowSubdomains-1; i++)
    theCopy[i] = theShadowSubdomains[i];

  if (theShadowSubdomains != 0)
    delete [] theShadowSubdomains;

  theCopy[numShadowSubdomains-1] = this;

  theShadowSubdomains = theCopy;

  // does nothing
  numLoadPatterns = 0;

  msgData(0) = ShadowActorSubdomain_setTag;
  msgData(1) = tag;
  this->sendID(msgData);
}


ShadowSubdomain::ShadowSubdomain(int tag, 
				 Channel &the_Channel,
				 FEM_ObjectBroker &theObjectBroker)
  :Shadow(the_Channel, theObjectBroker),
   Subdomain(tag),
   msgData(4),
   theElements(0,128),
   theNodes(0,128),
   theExternalNodes(0,128), 
   theLoadCases(0,128),
   numDOF(0),numElements(0),numNodes(0),numExternalNodes(0),
   numSPs(0),numMPs(0), buildRemote(false), gotRemoteData(false), 
   theFEele(0),
   theVector(0), theMatrix(0)
{

  numShadowSubdomains++;

  ShadowSubdomain **theCopy = new ShadowSubdomain *[numShadowSubdomains];

  for (int i = 0; i < numShadowSubdomains-1; i++)
    theCopy[i] = theShadowSubdomains[i];

  if (theShadowSubdomains != 0)
    delete [] theShadowSubdomains;

  theCopy[numShadowSubdomains-1] = this;

  theShadowSubdomains = theCopy;

  // does nothing
  numLoadPatterns = 0;
}

ShadowSubdomain::~ShadowSubdomain()    
{
    // send a message to the remote actor telling it to shut sown
    msgData(0) = ShadowActorSubdomain_DIE;
    this->sendID(msgData);
}


int
ShadowSubdomain::buildSubdomain(int numSubdomains, PartitionedModelBuilder &theBuilder)
{
  // send a message identify setting ModelBuilder and it's class tag
  buildRemote = true;
  gotRemoteData = false;

  msgData(0) = ShadowActorSubdomain_buildSubdomain;
  msgData(1) = theBuilder.getClassTag();
  msgData(2) = numSubdomains;
  msgData(3) = this->getTag();
  this->sendID(msgData);

  // send the builder
  this->sendObject(theBuilder);

  // mark the domain as having been modified
  this->domainChange();
  return 0;  
}


int
ShadowSubdomain::getRemoteData(void)
{
  if (buildRemote == true && gotRemoteData == false){
    msgData(0) = ShadowActorSubdomain_getRemoteData;
    this->sendID(msgData);

    this->recvID(msgData);
    numExternalNodes = msgData(0);
    numDOF = msgData(1);

opserr << "ShadowSubdomain::getRemoteData numExtNodes " << numExternalNodes << endln;    
    if (theExternalNodes.Size() != numExternalNodes)
      theExternalNodes[numExternalNodes-1] = 0; // this will resize
    if (theExternalNodes.Size() != numExternalNodes) {
      opserr << "ShadowSubdomain::getRemoteData(void) - need to resize the ID\n";
      return -1;
    }
    if (numExternalNodes != 0)
      this->recvID(theExternalNodes);
opserr << "ShadowSubdomain::getREmoteData " << theExternalNodes;    
  }

  gotRemoteData = true;
  return 0;
}

bool 
ShadowSubdomain::addElement(Element *theEle)
{
    int tag = theEle->getTag();

#ifdef _G3DEBUG    
	// do all the checking stuff
#endif

    msgData(0) = ShadowActorSubdomain_addElement;
    msgData(1) = theEle->getClassTag();
    msgData(2) = theEle->getDbTag();
    this->sendID(msgData);
    this->sendObject(*theEle);
    theElements[numElements] = tag;
    numElements++;
    this->domainChange();

    /*
    msgData(0) = 5;
    msgData(1) = 6;
    msgData(2) = 7;
    msgData(3) = 8;
    this->sendID(msgData);  
    this->recvID(msgData);  
    opserr << "ShadowSubdomain::addElement() : " << msgData;
    */

    delete theEle;
    
    return true;
}

bool 
ShadowSubdomain::addNode(Node *theNode)
{
    int tag = theNode->getTag();
#ifdef _G3DEBUG
	// do all the checking stuff
#endif
    msgData(0) = ShadowActorSubdomain_addNode;
    msgData(1) = theNode->getClassTag();
    msgData(2) = theNode->getDbTag();
    this->sendID(msgData);
    this->sendObject(*theNode);
    theNodes[numNodes] = tag;
    numNodes++;    
    this->domainChange();

    delete theNode;
    
    return true;
}

bool 
ShadowSubdomain::addExternalNode(Node *theNode)
{
    int tag = theNode->getTag();
#ifdef _G3DEBUG
	// do all the checking stuff
#endif

    msgData(0) = ShadowActorSubdomain_addExternalNode;
    msgData(1) = theNode->getClassTag();
    msgData(2) = theNode->getDbTag();
    this->sendID(msgData);
    this->sendObject(*theNode);
    theNodes[numNodes] = tag;
    theExternalNodes[numExternalNodes] = tag;    
    numNodes++;    
    numExternalNodes++;        
    numDOF += theNode->getNumberDOF();

    this->domainChange();

    return true;    
}

bool 
ShadowSubdomain::addSP_Constraint(SP_Constraint *theSP)
{

#ifdef _G3DEBUG
	// do all the checking stuff
#endif
    msgData(0) = ShadowActorSubdomain_addSP_Constraint;
    msgData(1) = theSP->getClassTag();
    msgData(2) = theSP->getDbTag();
    this->sendID(msgData);
    this->sendObject(*theSP);
    numSPs++;    
    this->domainChange();
    
    this->Subdomain::addSP_Constraint(theSP);
    return true;    
}

bool 
ShadowSubdomain::addMP_Constraint(MP_Constraint *theMP)    
{
#ifdef _G3DEBUG
	// do all the checking stuff
#endif
    msgData(0) = ShadowActorSubdomain_addMP_Constraint;
    msgData(1) = theMP->getClassTag();
    msgData(2) = theMP->getDbTag();
    this->sendID(msgData);
    this->sendObject(*theMP);
    numMPs++;    
    this->domainChange();
    
    this->Subdomain::addMP_Constraint(theMP);
    return true;    
}


bool 
ShadowSubdomain::addLoadPattern(LoadPattern *thePattern)    
{
#ifdef _G3DEBUG
	// do all the checking stuff
#endif
    msgData(0) = ShadowActorSubdomain_addLoadPattern;
    msgData(1) = thePattern->getClassTag();
    msgData(2) = thePattern->getDbTag();
    this->sendID(msgData);
    this->sendObject(*thePattern);
    this->domainChange();

    this->Subdomain::addLoadPattern(thePattern);
    numLoadPatterns++;
    return true;    
}



bool 
ShadowSubdomain::addSP_Constraint(SP_Constraint *theSP, int loadPattern)
{
#ifdef _G3DEBUG
	// do all the checking stuff
#endif

  LoadPattern *thePattern = this->Subdomain::getLoadPattern(loadPattern);
  if ((thePattern == 0) || (thePattern->addSP_Constraint(theSP) == false)) {
    opserr << "ShadowSubdomain::addSP_Constraint() - could not add SP_Constraint: " << *theSP;
    return false;
  }

  msgData(0) = ShadowActorSubdomain_addSP_ConstraintToPattern;
  msgData(1) = theSP->getClassTag();
  msgData(2) = theSP->getDbTag();
  msgData(3) = loadPattern;
  this->sendID(msgData);
  this->sendObject(*theSP);
  numSPs++;    
  this->domainChange();
  
  return true;    
}

bool 
ShadowSubdomain::addNodalLoad(NodalLoad *theLoad, int loadPattern)
{
#ifdef _G3DEBUG
    // do all the checking stuff
#endif
  LoadPattern *thePattern = this->Subdomain::getLoadPattern(loadPattern);
  if ((thePattern == 0) || (thePattern->addNodalLoad(theLoad) == false)) {
    opserr << "ShadowSubdomain::addNodalLoad() - could not add the load: " << *theLoad;
    return false;
  }

  msgData(0) = ShadowActorSubdomain_addNodalLoadToPattern;
  msgData(1) = theLoad->getClassTag();
  msgData(2) = theLoad->getDbTag();
  msgData(3) = loadPattern;
  this->sendID(msgData);
  this->sendObject(*theLoad);
  
  return true;    
}

bool 
ShadowSubdomain::addElementalLoad(ElementalLoad *theLoad, int loadPattern)
{
#ifdef _G3DEBUG
	// do all the checking stuff
#endif
  LoadPattern *thePattern = this->Subdomain::getLoadPattern(loadPattern);
  if ((thePattern == 0) || (thePattern->addElementalLoad(theLoad) == false)) {
    opserr << "ShadowSubdomain::addElementalLoad() - could not add the load: " << *theLoad;
    return false;
  }
  msgData(0) = ShadowActorSubdomain_addElementalLoadToPattern;
  msgData(1) = theLoad->getClassTag();
  msgData(2) = theLoad->getDbTag();
  msgData(3) = loadPattern;
  this->sendID(msgData);
  this->sendObject(*theLoad);

  return true;    
}

    
Element  *
ShadowSubdomain::removeElement(int tag)
{
    int loc = theElements.removeValue(tag);
    if (loc < 0)
	return 0;
    else { // the element is there go get it
	// send a message to remove the object from the actor
	msgData(0) = ShadowActorSubdomain_removeElement;
	msgData(1) = tag;
	this->sendID(msgData);

	numElements--;
	this->domainChange();	

	// get the element from the actor
	this->recvID(msgData);
	int theType = msgData(0);
	
	if (theType == -1) // the ele was not there!
	    return 0;    
    
	Element *theEle = theBroker->getNewElement(theType);
	if (theEle != 0) 
	    this->recvObject(*theEle);
    
	return theEle;
    }
}

Node 	 *
ShadowSubdomain::removeNode(int tag)    
{
    int loc = theNodes.removeValue(tag);
    if (loc < 0)
	return 0;
    else { // the node is there, go get it
	// send a message to actor requesting node
	msgData(0) = ShadowActorSubdomain_removeNode;
	msgData(1) = tag;
	this->sendID(msgData);


	numNodes--;
	this->domainChange();	
	// remove from external as well
	loc = theExternalNodes.removeValue(tag);
	if (loc >= 0)
	    numExternalNodes--;
	
	// receive the node from the actor
	this->recvID(msgData);
	int theType = msgData(0);

	if (theType == -1)
	    return 0;
    
	Node *theNode = theBroker->getNewNode(theType);
	if (theNode != 0) {
	    this->recvObject(*theNode);
	    if (loc >= 0)
		numDOF -= theNode->getNumberDOF();
	}
	return theNode;    
    }
}

SP_Constraint *
ShadowSubdomain::removeSP_Constraint(int tag)
{
    SP_Constraint *spPtr = this->Subdomain::removeSP_Constraint(tag);
    if (spPtr == 0)
	return 0;

    msgData(0) = ShadowActorSubdomain_removeSP_Constraint;
    msgData(1) = tag;
    
    this->sendID(msgData);
    numSPs--;
    
    return spPtr;
}

MP_Constraint *
ShadowSubdomain::removeMP_Constraint(int tag)    
{
    MP_Constraint *mpPtr = this->Subdomain::removeMP_Constraint(tag);
    if (mpPtr == 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_removeMP_Constraint;
    msgData(1) = tag;
    
    this->sendID(msgData);

    numMPs--;
    return mpPtr;
}

LoadPattern *
ShadowSubdomain::removeLoadPattern(int loadTag)
{
    LoadPattern *res = this->Subdomain::removeLoadPattern(loadTag);
    if (res == 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_removeLoadPattern;
    msgData(1) = loadTag;
    
    this->sendID(msgData);
    return res;
}

NodalLoad *
ShadowSubdomain::removeNodalLoad(int loadTag, int loadPattern)
{
    NodalLoad *res = this->Subdomain::removeNodalLoad(loadTag, loadPattern);
    if (res == 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_removeNodalLoadFromPattern;
    msgData(1) = loadTag;
    msgData(2) = loadPattern;
    
    this->sendID(msgData);
    return res;
}



ElementalLoad *
ShadowSubdomain::removeElementalLoad(int loadTag, int loadPattern)
{
    ElementalLoad *res = this->Subdomain::removeElementalLoad(loadTag, loadPattern);
    if (res == 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_removeElementalLoadFromPattern;
    msgData(1) = loadTag;
    msgData(2) = loadPattern;
    
    this->sendID(msgData);
    return res;
}

SP_Constraint *
ShadowSubdomain::removeSP_Constraint(int loadTag, int loadPattern)
{
    SP_Constraint *res = this->Subdomain::removeSP_Constraint(loadTag, loadPattern);
    if (res == 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_removeSP_ConstraintFromPattern;
    msgData(1) = loadTag;
    msgData(2) = loadPattern;
    
    this->sendID(msgData);
    return res;
}


ElementIter       &
ShadowSubdomain::getElements()
{
    opserr << "ShadowSubdomain::getElements() ";
    opserr << " - SHOULD NEVER BE CALLED - EXITING\n";
    exit(-1);

    // this will never be called - just for a strict compiler    
    ArrayOfTaggedObjects *theEror = new ArrayOfTaggedObjects(1);
    ElementIter *theIter = new SingleDomEleIter(theEror);
    return *theIter;
}

NodeIter          &
ShadowSubdomain::getNodes()
{
    opserr << "ShadowSubdomain::getNodes() ";
    opserr << " - SHOULD NEVER BE CALLED - EXITING\n";
    exit(-1);

    // this will never be called - just for a strict compiler
    ArrayOfTaggedObjects *theEror = new ArrayOfTaggedObjects(1);
    NodeIter *theIter = new SingleDomNodIter(theEror);
    return *theIter;
}

NodeIter &
ShadowSubdomain::getInternalNodeIter(void)
{
    opserr << "ShadowSubdomain::getInternalNodeIter() ";
    opserr << " - SHOULD NEVER BE CALLED - EXITING\n";
    exit(-1);

    // this will never be called - just for a strict compiler
    ArrayOfTaggedObjects *theEror = new ArrayOfTaggedObjects(1);
    NodeIter *theIter = new SingleDomNodIter(theEror);
    return *theIter;
}

NodeIter 	  &
ShadowSubdomain::getExternalNodeIter(void)
{
    opserr << "ShadowSubdomain::getExternalNodeIter() ";
    opserr << " - SHOULD NEVER BE CALLED - EXITING\n";
    exit(-1);

    // this will never be called - just for a strict compiler
    ArrayOfTaggedObjects *theEror = new ArrayOfTaggedObjects(1);
    NodeIter *theIter = new SingleDomNodIter(theEror);
    return *theIter;
}

    
Element *
ShadowSubdomain::getElementPtr(int tag) 
{
    if (theElements.getLocation(tag) < 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_getElementPtr;
    msgData(1) = tag;
    this->sendID(msgData);
    this->recvID(msgData);
    int theType = msgData(0);
    
    Element *theEle = theBroker->getNewElement(theType);
    if (theEle != 0) {
	this->recvObject(*theEle);
    }
    opserr << "ShadowSubdomain::getElementPtr() ";
    opserr << " - SHOULD BE AVOIDED IF POSSIBLE \n";
    
    return theEle;
}

Node  *
ShadowSubdomain::getNodePtr(int tag)
{
    if (theNodes.getLocation(tag) < 0)
	return 0;
    
    msgData(0) = ShadowActorSubdomain_getNodePtr;
    msgData(1) = tag;
    this->sendID(msgData);
    this->recvID(msgData);
    int theType = msgData(0);
    
    Node *theNod = theBroker->getNewNode(theType);
    if (theNod != 0) {
	this->recvObject(*theNod);
    }
    opserr << "ShadowSubdomain::getNodPtr() ";
    opserr << " - SHOULD BE AVOIDED IF POSSIBLE \n";
    
    return theNod;
}

int 
ShadowSubdomain::getNumElements(void) const
{
    return numElements;
}

int 
ShadowSubdomain::getNumLoadPatterns(void) const
{
    return numLoadPatterns;
}

int 
ShadowSubdomain::getNumNodes(void) const
{
    return numNodes;
}

int 
ShadowSubdomain::getNumSPs(void) const
{
    return numSPs;
}

int 
ShadowSubdomain::getNumMPs(void) const
{
    return numMPs;
}

    
Graph &
ShadowSubdomain::getElementGraph(void)
{
    opserr << "ShadowSubdomain::getElementGraph() ";
    opserr << " - NOT YET IMPLEMENTED - EXITING\n";
    exit(-1);

    // this will never be called, needed for a strict compiler
    Graph *theGraph = new Graph;
    return *theGraph;
}

Graph             &
ShadowSubdomain::getNodeGraph(void)
{
    opserr << "ShadowSubdomain::getNodeGraph() ";
    opserr << " - NOT YET IMPLEMENTED - EXITING\n";
    exit(-1);

    // this will never be called, needed for a strict compiler
    Graph *theGraph = new Graph;
    return *theGraph;
}

    
void 
ShadowSubdomain::applyLoad(double time)
{
  DomainDecompositionAnalysis *theDDA = this->getDDAnalysis();
  if (theDDA != 0 && theDDA->doesIndependentAnalysis() != true) {
    msgData(0) = ShadowActorSubdomain_applyLoad;
    Vector data(4);
    data(0) = time;
  
    this->sendID(msgData);
    this->sendVector(data);    
  }
}


void 
ShadowSubdomain::setCommitTag(int newTag)
{
    msgData(0) = ShadowActorSubdomain_setCommitTag;
    msgData(1) = newTag;

    this->sendID(msgData);
}

void 
ShadowSubdomain::setCurrentTime(double time)
{
    msgData(0) = ShadowActorSubdomain_setCurrentTime;
    Vector data(1);
    data(0) = time;
    
    this->sendID(msgData);
    this->sendVector(data);    

}

void 
ShadowSubdomain::setCommittedTime(double time)
{
    msgData(0) = ShadowActorSubdomain_setCommittedTime;
    Vector data(1);
    data(0) = time;
    
    this->sendID(msgData);
    this->sendVector(data);    

}

void 
ShadowSubdomain::setLoadConstant(void)
{
    msgData(0) = ShadowActorSubdomain_setLoadConstant;
    
    this->sendID(msgData);
}


int
ShadowSubdomain::update(void)
{
  DomainDecompositionAnalysis *theDDA = this->getDDAnalysis();
  if (theDDA != 0 && theDDA->doesIndependentAnalysis() != true) {
    msgData(0) =  ShadowActorSubdomain_update;
    this->sendID(msgData);
  }
  return 0;
}

int
ShadowSubdomain::commit(void)
{
  DomainDecompositionAnalysis *theDDA = this->getDDAnalysis();
  if (theDDA != 0 && theDDA->doesIndependentAnalysis() != true) {
    msgData(0) = ShadowActorSubdomain_commit;
    this->sendID(msgData);
    return 0;
  }
  return 0;
}

int
ShadowSubdomain::revertToLastCommit(void)
{
  DomainDecompositionAnalysis *theDDA = this->getDDAnalysis();
  if (theDDA != 0 && theDDA->doesIndependentAnalysis() != true) {
    msgData(0) = ShadowActorSubdomain_revertToLastCommit;
    this->sendID(msgData);
    return 0;
  }
  return 0;
}

int
ShadowSubdomain::revertToStart(void)
{
  msgData(0) = ShadowActorSubdomain_revertToStart;
  this->sendID(msgData);
  return 0;
}



void 
ShadowSubdomain::setDomainDecompAnalysis(DomainDecompositionAnalysis &theDDAnalysis)
{
    msgData(0) = ShadowActorSubdomain_setDomainDecompAnalysis;
    msgData(1) = theDDAnalysis.getClassTag();
    
    this->sendID(msgData);
    this->sendObject(theDDAnalysis);

    this->Subdomain::setDomainDecompAnalysis(theDDAnalysis);
}

int 
ShadowSubdomain::setAnalysisAlgorithm(EquiSolnAlgo &theAlgorithm)
{
    msgData(0) = ShadowActorSubdomain_setAnalysisAlgorithm;
    msgData(1) = theAlgorithm.getClassTag();
    
    this->sendID(msgData);
    this->sendObject(theAlgorithm);
    return 0;
}

int 
ShadowSubdomain::setAnalysisIntegrator(IncrementalIntegrator &theIntegrator)
{
    msgData(0) = ShadowActorSubdomain_setAnalysisIntegrator;
    msgData(1) = theIntegrator.getClassTag();
    
    this->sendID(msgData);
    this->sendObject(theIntegrator);
    return 0;
}

int 
ShadowSubdomain::setAnalysisLinearSOE(LinearSOE &theSOE)
{
    msgData(0) = ShadowActorSubdomain_setAnalysisLinearSOE;
    msgData(1) = theSOE.getClassTag();
    
    this->sendID(msgData);
    this->sendObject(theSOE);
    return 0;
}



int
ShadowSubdomain::invokeChangeOnAnalysis(void)
{
    msgData(0) = ShadowActorSubdomain_invokeChangeOnAnalysis;
    this->sendID(msgData);
    
    if (theVector == 0)
	theVector = new Vector(numDOF);
    else if (theVector->Size() != numDOF) {
	delete theVector;
	theVector = new Vector(numDOF);
    }
    
    if (theMatrix == 0)
	theMatrix = new Matrix(numDOF,numDOF);
    else if (theMatrix->noRows() != numDOF) {
	delete theMatrix;
	theMatrix = new Matrix(numDOF,numDOF);
    }    
    
    return 0;
}

void 
ShadowSubdomain::clearAnalysis(void)
{
    msgData(0) = ShadowActorSubdomain_clearAnalysis;
    this->sendID(msgData);
}

int 	
ShadowSubdomain::getNumExternalNodes(void) const    
{
  if (gotRemoteData == false && buildRemote == true) {    
    opserr << "WARNING: ShadowSubdomain::getNumExternalNodes()";
    opserr << " - not yet received the data\n";
  }
  
  return numExternalNodes;
}

const ID   &
ShadowSubdomain::getExternalNodes(void)
{
  // if the subdoamin was built remotly need to get it's data
  if (gotRemoteData == false && buildRemote == true)
    this->getRemoteData();

    return theExternalNodes;
}

int 	
ShadowSubdomain::getNumDOF(void)
{
  // if the subdoamin was built remotly need to get it's data
  if (gotRemoteData == false && buildRemote == true)
    this->getRemoteData();

    return numDOF;
}


const Matrix &
ShadowSubdomain::getTang(void)    
{
  // if the subdoamin was built remotly need to get it's data
  if (gotRemoteData == false && buildRemote == true)
    this->getRemoteData();

    msgData(0) =  ShadowActorSubdomain_getTang;
    this->sendID(msgData);
    
    if (theMatrix == 0)
	theMatrix = new Matrix(numDOF,numDOF);
    else if (theMatrix->noRows() != numDOF) {
	delete theMatrix;
	theMatrix = new Matrix(numDOF,numDOF);
    }    
    
    this->recvMatrix(*theMatrix);
    return *theMatrix;
}



const Vector &
ShadowSubdomain::getResistingForce(void)    
{
  // if the subdoamin was built remotly need to get it's data
  if (gotRemoteData == false && buildRemote == true)
    this->getRemoteData();

    msgData(0) = ShadowActorSubdomain_getResistingForce;
    this->sendID(msgData);
    
    if (theVector == 0)
	theVector = new Vector(numDOF);
    else if (theVector->Size() != numDOF) {
	delete theVector;
	theVector = new Vector(numDOF);
    }    
    
    this->recvVector(*theVector);
    return *theVector;
}


int  	  
ShadowSubdomain::computeTang(void)
{
    count++;

    if (count == 1) {
      msgData(0) = ShadowActorSubdomain_computeTang;
      msgData(1) = this->getTag();
      this->sendID(msgData);

      for (int i = 0; i < numShadowSubdomains; i++) {
	ShadowSubdomain *theShadow = theShadowSubdomains[i];
	if (theShadow != this)
	  theShadow->computeTang();
      }
    }
    else if (count <= numShadowSubdomains) {
      msgData(0) = ShadowActorSubdomain_computeTang;
      msgData(1) = this->getTag();
      this->sendID(msgData);
    }
    else if (count == 2*numShadowSubdomains - 1)
      count = 0;
    
    return 0;
}

int  
ShadowSubdomain::computeResidual(void)
{
    count++;

    if (count == 1) {
      msgData(0) = ShadowActorSubdomain_computeResidual;
      this->sendID(msgData);

      for (int i = 0; i < numShadowSubdomains; i++) {
	ShadowSubdomain *theShadow = theShadowSubdomains[i];
	if (theShadow != this)
	  theShadow->computeResidual();
      }
    }
    else if (count <= numShadowSubdomains) {
      msgData(0) = ShadowActorSubdomain_computeResidual;
      this->sendID(msgData);
    }
    else if (count == 2*numShadowSubdomains - 1)
      count = 0;

    return 0;
}



const Vector &
ShadowSubdomain::getLastExternalSysResponse(void)
{
    opserr << "ShadowSubdomain::getLastExternalSysResponse() ";
    opserr << " SHOULD NEVER BE CALLED\n";
    exit(0);

    // this should never be called, provided for a strict compiler
    Vector *theVect = new Vector(0);
    return *theVect;
}

int 
ShadowSubdomain::computeNodalResponse(void)    
{

  DomainDecompositionAnalysis *theDDA = this->getDDAnalysis();
  if (theDDA != 0 && theDDA->doesIndependentAnalysis() != true) {
    FE_Element *theFePtr = this->getFE_ElementPtr();

    if (theFePtr != 0) {

      const Vector &lastChange = theFePtr->getLastResponse();
      msgData(0) =  ShadowActorSubdomain_computeNodalResponse;
      msgData(1) = lastChange.Size();
      if (numDOF != msgData(1)) {
	opserr << "ShadowSubdomain::update(void)";
	opserr << " - numDOF " << numDOF << " and size of Vector ";
	opserr << msgData(1) << "do not agree?\n";
	numDOF = msgData(1);
      }
      this->sendID(msgData);
      Vector theChange(lastChange);
      this->sendVector(theChange);
    }
  }
  
  return 0;
}


int 
ShadowSubdomain::newStep(double dT)    
{
  opserr << "ShadowSubdomain::newStep(double dT) -START\n";
    msgData(0) =  ShadowActorSubdomain_newStep;
    this->sendID(msgData);
    static Vector timeStep(4);
    timeStep(0) = dT;
    this->sendVector(timeStep);
  opserr << "ShadowSubdomain::newStep(double dT) -FINISH\n";
    return 0;
}


double
ShadowSubdomain::getCost(void)    
{
  /*
    msgData(0) = ShadowActorSubdomain_getCost;
    
    this->sendID(msgData);
    Vector cost(2);
    this->recvVector(cost);
    return cost(0);
  */
  return 0.0;
}


int 
ShadowSubdomain::sendSelf(int cTag, Channel &the_Channel)
{
    opserr << "ShadowSubdomain::sendSelf() ";
    opserr << " - NOT YET IMPLEMENTED\n";
    return -1;
}

int 
ShadowSubdomain::recvSelf(int cTag, Channel &the_Channel, 
			  FEM_ObjectBroker &the_Broker)    
{
    opserr << "ShadowSubdomain::recvSelf() ";
    opserr << " - NOT YET IMPLEMENTED\n";
    return -1;
}


void 
ShadowSubdomain::Print(OPS_Stream &s, int flag)
{
    msgData(0) = ShadowActorSubdomain_Print;
    
    this->sendID(msgData);
    this->recvID(msgData);
}


int 
ShadowSubdomain::buildEleGraph(Graph *theEleGraph)
{
    opserr << "ShadowSubdomain::buildEleGraph() ";
    opserr << " - NOT YET IMPLEMENTED\n";
    return -1;
}

int 
ShadowSubdomain::buildNodeGraph(Graph *theNodeGraph)
{
    opserr << "ShadowSubdomain::buildNodeGraph() ";
    opserr << " - NOT YET IMPLEMENTED\n";
    return -1;
}

int
ShadowSubdomain::buildMap(void)
{
  mapBuilt = true;
  return 0;
}













