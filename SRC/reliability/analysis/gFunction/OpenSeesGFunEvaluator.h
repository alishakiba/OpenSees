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
                                                                        
// $Revision: 1.12 $
// $Date: 2008-05-27 20:04:30 $
// $Source: /usr/local/cvs/OpenSees/SRC/reliability/analysis/gFunction/OpenSeesGFunEvaluator.h,v $


//
// Written by Terje Haukaas (haukaas@ce.berkeley.edu)
//

#ifndef OpenSeesGFunEvaluator_h
#define OpenSeesGFunEvaluator_h

#include <GFunEvaluator.h>
#include <Vector.h>
#include <Domain.h>
#include <ReliabilityDomain.h>
#include <tcl.h>

//#include <fstream>
//using std::ofstream;


class OpenSeesGFunEvaluator : public GFunEvaluator
{

 public:
  OpenSeesGFunEvaluator(Tcl_Interp *passedTclInterp,
			ReliabilityDomain *passedReliabilityDomain,
			Domain *passedOpenSeesDomain,
			TCL_Char *fileName);
  OpenSeesGFunEvaluator(Tcl_Interp *passedTclInterp,
			ReliabilityDomain *passedReliabilityDomain,
			Domain *passedOpenSeesDomain,
			int nsteps, double dt);
  ~OpenSeesGFunEvaluator();
  
  int		runGFunAnalysis(const Vector &x);
  int		tokenizeSpecials(TCL_Char *theExpression, Tcl_Obj *passedList);
  
  void		setNsteps(int nsteps);
  double	getDt();

  double	getG2(double g, double littleDt);
	
  
 protected:
  
 private:
	char fileName[256];
	int nsteps;
	double dt;
	
};

#endif
