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
                                                                        
// $Revision: 1.5 $
// $Date: 2003-02-14 23:00:48 $
// $Source: /usr/local/cvs/OpenSees/SRC/analysis/integrator/IncrementalIntegrator.cpp,v $
                                                                        
                                                                        
// File: ~/analysis/integrator/IncrementalIntegrator.C
// 
// Written: fmk 
// Created: 11/96
// Revision: A
//
// Description: This file contains the implementation of IncrementalIntegrator.
//
// What: "@(#) IncrementalIntegrator.C, revA"

#include <IncrementalIntegrator.h>
#include <FE_Element.h>
#include <LinearSOE.h>
#include <AnalysisModel.h>
#include <Vector.h>
#include <DOF_Group.h>
#include <FE_EleIter.h>
#include <DOF_GrpIter.h>

IncrementalIntegrator::IncrementalIntegrator(int clasTag)
:Integrator(clasTag),
 statusFlag(CURRENT_TANGENT),
 theSOE(0),theAnalysisModel(0)
{

}

IncrementalIntegrator::~IncrementalIntegrator()
{

}

void
IncrementalIntegrator::setLinks(AnalysisModel &theModel, LinearSOE &theLinSOE)
{
    theAnalysisModel = &theModel;
    theSOE = &theLinSOE;
}


int 
IncrementalIntegrator::formTangent(int statFlag)
{
    int result = 0;
    statusFlag = statFlag;

    if (theAnalysisModel == 0 || theSOE == 0) {
	opserr << "WARNING IncrementalIntegrator::formTangent() -";
	opserr << " no AnalysisModel or LinearSOE have been set\n";
	return -1;
    }

    // zero the A matrix of the linearSOE
    theSOE->zeroA();
    
    // the loops to form and add the tangents are broken into two for 
    // efficiency when performing parallel computations - CHANGE

    // loop through the FE_Elements adding their contributions to the tangent
    FE_Element *elePtr;
    FE_EleIter &theEles2 = theAnalysisModel->getFEs();    
    while((elePtr = theEles2()) != 0)     
	if (theSOE->addA(elePtr->getTangent(this),elePtr->getID()) < 0) {
	    opserr << "WARNING IncrementalIntegrator::formTangent -";
	    opserr << " failed in addA for ID " << elePtr->getID();	    
	    result = -3;
	}

    return result;
}


int 
IncrementalIntegrator::formUnbalance(void)
{
    if (theAnalysisModel == 0 || theSOE == 0) {
	opserr << "WARNING IncrementalIntegrator::formUnbalance -";
	opserr << " no AnalysisModel or LinearSOE has been set\n";
	return -1;
    }
    
    theSOE->zeroB();
    
    if (this->formElementResidual() < 0) {
	opserr << "WARNING IncrementalIntegrator::formUnbalance ";
	opserr << " - this->formElementResidual failed\n";
	return -1;
    }
    
    if (this->formNodalUnbalance() < 0) {
	opserr << "WARNING IncrementalIntegrator::formUnbalance ";
	opserr << " - this->formNodalUnbalance failed\n";
	return -2;
    }    

    return 0;
}
    
int
IncrementalIntegrator::getLastResponse(Vector &result, const ID &id)
{
  
    if (theSOE == 0) {
	opserr << "WARNING IncrementalIntegrator::getLastResponse() -";
	opserr << "no LineaerSOE object associated with this object\n";	
	return -1;
    }

    int res = 0; 
    int size = theSOE->getNumEqn() -1;
    const Vector &X = theSOE->getX();
    for (int i=0; i<id.Size(); i++) {
	int loc = id(i);
	if (loc < 0 )
	  result(i) = 0.0;
	else if (loc <= size) {
	  result(i) = X(loc);	
	}
	else {
	    opserr << "WARNING IncrementalIntegrator::getLastResponse() -";
	    opserr << "location " << loc << "in ID ouside bounds ";
	    opserr << size << "\n";	
	    res = -2;
	}
    }	    
    return res;
}


int
IncrementalIntegrator::newStep(double deltaT)
{
  return 0;
}


int
IncrementalIntegrator::initialize(void)
{
  return 0;
}

int
IncrementalIntegrator::commit(void) 
{
    if (theAnalysisModel == 0) {
	opserr << "WARNING IncrementalIntegrator::commit() -";
	opserr << "no AnalysisModel object associated with this object\n";	
	return -1;
    }    

    return theAnalysisModel->commitDomain();
}   


int
IncrementalIntegrator::revertToLastStep(void) 
{
  return 0;
}   


LinearSOE *
IncrementalIntegrator::getLinearSOEPtr(void) const
{
    return theSOE;
}   

AnalysisModel *
IncrementalIntegrator::getAnalysisModelPtr(void) const
{
    return theAnalysisModel;
}

int 
IncrementalIntegrator::formNodalUnbalance(void)
{
    // loop through the DOF_Groups and add the unbalance
    DOF_GrpIter &theDOFs = theAnalysisModel->getDOFs();
    DOF_Group *dofPtr;
    int res = 0;
    while ((dofPtr = theDOFs()) != 0) {
	if (theSOE->addB(dofPtr->getUnbalance(this),dofPtr->getID()) <0) {
	    opserr << "WARNING IncrementalIntegrator::formNodalUnbalance -";
	    opserr << " failed in addB for ID " << dofPtr->getID();
	    res = -2;
	}
    }
	
    return res;
}

int 
IncrementalIntegrator::formElementResidual(void)
{
    // loop through the FE_Elements and add the residual
    FE_Element *elePtr;

    int res = 0;    

    FE_EleIter &theEles2 = theAnalysisModel->getFEs();    
    while((elePtr = theEles2()) != 0) {
	if (theSOE->addB(elePtr->getResidual(this),elePtr->getID()) <0) {
	    opserr << "WARNING IncrementalIntegrator::formElementResidual -";
	    opserr << " failed in addB for ID " << elePtr->getID();
	    res = -2;
	}
    }

    return res;	    
}

