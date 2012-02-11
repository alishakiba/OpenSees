/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 2001, The Regents of the University of California    **
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
** Reliability module developed by:                                   **
**   Terje Haukaas (haukaas@ce.berkeley.edu)                          **
**   Armen Der Kiureghian (adk@ce.berkeley.edu)                       **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision: 1.9 $
// $Date: 2008-05-27 20:04:30 $
// $Source: /usr/local/cvs/OpenSees/SRC/reliability/analysis/analysis/FOSMAnalysis.cpp,v $


//
// Written by Terje Haukaas (haukaas@ce.berkeley.edu)
//

#include <FOSMAnalysis.h>
#include <ReliabilityAnalysis.h>
#include <ReliabilityDomain.h>
#include <RandomVariablePositioner.h>
#include <FunctionEvaluator.h>
#include <GradientEvaluator.h>
#include <Matrix.h>
#include <Vector.h>
#include <tcl.h>
#include <math.h>

#include <RandomVariableIter.h>
#include <LimitStateFunctionIter.h>
#include <CorrelationCoefficientIter.h>

#include <fstream>
#include <iomanip>
#include <iostream>

using std::ifstream;
using std::ios;
using std::setw;
using std::setprecision;
using std::setiosflags;


FOSMAnalysis::FOSMAnalysis(ReliabilityDomain *passedReliabilityDomain,
			   FunctionEvaluator *passedGFunEvaluator,
			   GradientEvaluator *passedGradGEvaluator,
			   Tcl_Interp *passedTclInterp,
			   TCL_Char *passedFileName)
:ReliabilityAnalysis()
{
	theReliabilityDomain	= passedReliabilityDomain;
	theGFunEvaluator		= passedGFunEvaluator;
	theGradGEvaluator = passedGradGEvaluator;
	theTclInterp			= passedTclInterp;
	strcpy(fileName,passedFileName);
}


FOSMAnalysis::~FOSMAnalysis()
{

}



int 
FOSMAnalysis::analyze(void)
{

	// Alert the user that the FORM analysis has started
	opserr << "FOSM Analysis is running ... " << endln;

	// Open output file
	ofstream outputFile( fileName, ios::out );

	// Get number of random variables and limit-state function
	int nrv = theReliabilityDomain->getNumberOfRandomVariables();
	int numLsf = theReliabilityDomain->getNumberOfLimitStateFunctions();
	LimitStateFunction *theLSF;
	
	// Get mean point
	Vector meanVector(nrv);
	// Establish vector of standard deviations
	Vector stdvVector(nrv);

    // note that now reliability domain components are persistent, so it matters if FOSM is run
    // after a FORM analysis, for example.  Reset to mean value because this is by definition
	RandomVariableIter rvIter = theReliabilityDomain->getRandomVariables();
	RandomVariable *aRandomVariable;
	//for (int i=1; i<=nrv; i++) {
	while ((aRandomVariable = rvIter()) != 0) {
		int tag = aRandomVariable->getTag();
		int i = theReliabilityDomain->getRandomVariableIndex(tag);
		meanVector(i) = aRandomVariable->getMean();
		stdvVector(i) = aRandomVariable->getStdv();
        
        // make sure the RV starts at mean
        aRandomVariable->setCurrentValue(meanVector(i));
	}
    
	// Perform analysis using mean vector
	Vector meanEstimates(numLsf);
    
    // set values in the variable namespace
    if (theGFunEvaluator->setVariables(meanVector) < 0) {
        opserr << "FOSMAnalysis::analyze() - " << endln
            << " could not set variables in namespace. " << endln;
        return -1;
    }
    
    // run analysis
	if (theGFunEvaluator->runAnalysis(meanVector) < 0) {
		opserr << "FOSMAnalysis::analyze() - " << endln
			<< " could not run analysis to evaluate limit-state function. " << endln;
		return -1;
	}
	
	// Loop over number of limit-state functions and evaluate
	//LimitStateFunctionIter lsfIter = theReliabilityDomain->getLimitStateFunctions();
	//while ((theLSF = lsfIter()) != 0) {
	for (int lsf = 0; lsf < numLsf; lsf++ ) {
		theLSF = theReliabilityDomain->getLimitStateFunctionPtrFromIndex(lsf);
		int lsfTag = theLSF->getTag();
		
		// Set tag of "active" limit-state function
		theReliabilityDomain->setTagOfActiveLimitStateFunction(lsfTag);
		
		// set namespace variable for tcl functions
		Tcl_SetVar2Ex(theTclInterp,"RELIABILITY_lsf",NULL,Tcl_NewIntObj(lsfTag),TCL_NAMESPACE_ONLY);
        
        // set and evaluate LSF
        const char *lsfExpression = theLSF->getExpression();
        theGFunEvaluator->setExpression(lsfExpression);
		
		meanEstimates(lsf) = theGFunEvaluator->evaluateExpression();
	}
    

	// Establish covariance matrix
	Matrix covMatrix(nrv,nrv);
	for (int i = 0; i < nrv; i++) {
	  covMatrix(i,i) = stdvVector(i)*stdvVector(i);
	}

	double covariance, correlation;
	int rv1, rv2;
	RandomVariable *rv1Ptr;
	RandomVariable *rv2Ptr;
	CorrelationCoefficient *theCorrelationCoefficient;
	CorrelationCoefficientIter ccIter = theReliabilityDomain->getCorrelationCoefficients();
	//for (i=1 ; i<=ncorr ; i++) {
	while ((theCorrelationCoefficient = ccIter()) != 0) {
		correlation = theCorrelationCoefficient->getCorrelation();
		rv1 = theCorrelationCoefficient->getRv1();
		rv2 = theCorrelationCoefficient->getRv2();
		rv1Ptr = theReliabilityDomain->getRandomVariablePtr(rv1);
		if (rv1Ptr == 0) {
		  opserr << "FOSMAnalysis::analyze -- random variable with tag " << rv1 << " not found in domain" << endln;
		  return -1;
		}
		rv2Ptr = theReliabilityDomain->getRandomVariablePtr(rv2);
		if (rv2Ptr == 0) {
		  opserr << "FOSMAnalysis::analyze -- random variable with tag " << rv2 << " not found in domain" << endln;
		  return -1;
		}
		//int i1 = rv1Ptr->getIndex();
		//int i2 = rv2Ptr->getIndex();
		int i1 = theReliabilityDomain->getRandomVariableIndex(rv1);
		int i2 = theReliabilityDomain->getRandomVariableIndex(rv2);
		covariance = correlation*stdvVector(i1)*stdvVector(i2);
		covMatrix(i1,i2) = covariance;
		covMatrix(i2,i1) = covariance;
	}


	// Post-processing loop over limit-state functions
	Vector responseStdv(numLsf);
	Vector gradient(nrv);
	double responseVariance;
	
    // Store gradients for now (probably a better way than this, but gets rid of old allGradG)
	Matrix matrixOfGradientVectors(nrv,numLsf);
    
	//lsfIter.reset();
	//while ((theLSF = lsfIter()) != 0) {
		//int lsf = theLSF->getTag();
		//int j = theLSF->getIndex();
		//int j = theReliabilityDomain->getLimitStateFunctionIndex(lsf);

	for (int lsf = 0; lsf < numLsf; lsf++ ) {
		theLSF = theReliabilityDomain->getLimitStateFunctionPtrFromIndex(lsf);
		int lsfTag = theLSF->getTag();
		
		// Set tag of active limit-state function
		theReliabilityDomain->setTagOfActiveLimitStateFunction(lsfTag);
        
        // Gradient in original space
		if (theGradGEvaluator->computeGradient( meanEstimates(lsf) ) < 0 ) {
			opserr << "FOSMAnalysis:analyze() - " << endln
                << " could not compute gradients of the limit-state function. " << endln;
			return -1;
		}
		gradient = theGradGEvaluator->getGradient();

		// Estimate of standard deviation of response
		responseVariance = (covMatrix^gradient)^gradient;
		if (responseVariance <= 0.0) {
			opserr << "ERROR: Response variance of limit-state function number "<< lsfTag
				<< " is zero! " << endln;
		}
		else {
			responseStdv(lsf) = sqrt(responseVariance);
		}

		// Compute importance measure (dgdx*stdv)
		Vector importance(nrv);
		for (int i = 0; i < nrv ; i++) {
            importance(i) = gradient(i) * stdvVector(i);
            matrixOfGradientVectors(i,lsf) = gradient(i);
        }

		importance /= importance.Norm();

	
		// Print FOSM results to the output file
		outputFile << "#######################################################################" << endln;
		outputFile << "#  FOSM ANALYSIS RESULTS, LIMIT-STATE FUNCTION NUMBER   "
			<<setiosflags(ios::left)<<setprecision(1)<<setw(4)<<lsfTag <<"          #" << endln;
		outputFile << "#                                                                     #" << endln;
		
		outputFile << "#  Estimated mean: .................................... " 
			<<setiosflags(ios::left)<<setprecision(5)<<setw(12)<<meanEstimates(lsf) 
			<< "  #" << endln;
		outputFile << "#  Estimated standard deviation: ...................... " 
			<<setiosflags(ios::left)<<setprecision(5)<<setw(12)<<responseStdv(lsf) 
			<< "  #" << endln;
		outputFile << "#                                                                     #" << endln;
		outputFile << "#      Rvtag        Importance measure (dgdx*stdv)                    #" << endln;
		outputFile.setf( ios::scientific, ios::floatfield );
		rvIter.reset();
		//for (int i=0;  i<nrv; i++) {
		while ((aRandomVariable = rvIter()) != 0) {
		  int tag = aRandomVariable->getTag();
		  int i = theReliabilityDomain->getRandomVariableIndex(tag);
		  outputFile << "#       " <<setw(3)<< tag <<"              ";
		  outputFile << "";
		  if (importance(i) < 0.0)
		    outputFile << "-"; 
		  else
		    outputFile << " "; 
		  outputFile <<setprecision(3)<<setw(11)<<fabs(importance(i));
		  outputFile << "                                 #" << endln;
		}
		outputFile << "#                                                                     #" << endln;
		outputFile << "#######################################################################" << endln << endln << endln;
		outputFile.flush();

	}
    
    
	// Estimation of response covariance matrix
	Matrix responseCovMatrix(numLsf,numLsf);
	double responseCovariance;
	Vector gradientVector1(nrv), gradientVector2(nrv);
	for (int i = 0; i < numLsf; i++) {
	  for (int k = 0; k < nrv; k++) {
	    gradientVector1(k) = matrixOfGradientVectors(k,i);
	  }
	  for (int j = i; j < numLsf; j++) {
	    for (int k = 0; k < nrv; k++) {
	      gradientVector2(k) = matrixOfGradientVectors(k,j);
	    }
	    responseCovariance = (covMatrix^gradientVector1)^gradientVector2;
	    responseCovMatrix(i,j) = responseCovariance;
	  }
	}
	for (int i = 0 ; i < numLsf; i++) {
	  for (int j = 0; j < i; j++) {
	    responseCovMatrix(i,j) = responseCovMatrix(j,i);
	  }
	}


	// Corresponding correlation matrix
	Matrix correlationMatrix(numLsf,numLsf);
	for (int i = 0; i < numLsf; i++) {
	  for (int j = i; j < numLsf; j++) {
	    correlationMatrix(i,j) = responseCovMatrix(i,j)/(responseStdv(i)*responseStdv(j));
	  }
	}
	for (int i = 0; i < numLsf; i++) {
	  for (int j = 0; j < i; j++) {
	    correlationMatrix(i,j) = correlationMatrix(j,i);
	  }
	}

	
	// Print correlation results
	outputFile << "#######################################################################" << endln;
	outputFile << "#  RESPONSE CORRELATION COEFFICIENTS                                  #" << endln;
	outputFile << "#                                                                     #" << endln;
	if (numLsf <=1) {
		outputFile << "#  Only one limit-state function!                                     #" << endln;
	}
	else {
		outputFile << "#   gFun   gFun     Correlation                                       #" << endln;
		outputFile.setf(ios::fixed, ios::floatfield);

		for (int lsfi = 0; lsfi < numLsf; lsfi++ ) {
			theLSF = theReliabilityDomain->getLimitStateFunctionPtrFromIndex(lsfi);
			int iTag = theLSF->getTag();

			for (int lsfj = lsfi+1; lsfj < numLsf; lsfj++ ) {
				theLSF = theReliabilityDomain->getLimitStateFunctionPtrFromIndex(lsfj);
				int jTag = theLSF->getTag();
				
				//				outputFile.setf(ios::fixed, ios::floatfield);
				outputFile << "#    " <<setw(3)<<iTag<<"    "<<setw(3)<<jTag<<"     ";
				if (correlationMatrix(lsfi,lsfj) < 0.0)
					outputFile << "-";
				else
					outputFile << " ";
					
				//				outputFile.setf(ios::scientific, ios::floatfield);
				outputFile <<setprecision(7)<<setw(11)<<fabs(correlationMatrix(lsfi,lsfj));
				outputFile << "                                      #" << endln;
			}
		}
	}
	outputFile << "#                                                                     #" << endln;
	outputFile << "#######################################################################" << endln << endln << endln;


	// Print summary of results to screen (more here!!!)
	opserr << "FOSMAnalysis completed." << endln;


	return 0;
}

