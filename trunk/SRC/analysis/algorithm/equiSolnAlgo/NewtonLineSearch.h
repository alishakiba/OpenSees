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

// $Revision: 1.1 $
// $Date: 2000-12-12 06:20:53 $
// $Source: /usr/local/cvs/OpenSees/SRC/analysis/algorithm/equiSolnAlgo/NewtonLineSearch.h,v $

// Written: fmk 
// Created: 11/96 
// Modified: Ed "C++" Love 10/00 to perform the line search
//

// Description: This file contains the class definition for 
// NewtonLineSearch. NewtonLineSearch is a class which performs a Newton-Raphson 
// with line search solution algorithm in solving the equations as outline in
// Crissfields book.
// 
// What: "@(#)NewtonLineSearch.h, revA"

#ifndef NewtonLineSearch_h
#define NewtonLineSearch_h

#include <EquiSolnAlgo.h>
#include <Vector.h>

class NewtonLineSearch: public EquiSolnAlgo
{
  public:
    NewtonLineSearch( ) ;    
    NewtonLineSearch(ConvergenceTest &theTest, double LineSearchTolerance ) ;
    ~NewtonLineSearch( ) ;

    int solveCurrentStep(void) ;    
    void setTest(ConvergenceTest &theNewTest) ;
    ConvergenceTest *getTest(void) ;     
    
    virtual int sendSelf(int commitTag, Channel &theChannel) ;
    virtual int recvSelf(int commitTag, Channel &theChannel, 
			 FEM_ObjectBroker &theBroker) ;

    void Print(ostream &s, int flag =0) ;    
    
  protected:
    
  private:

    ConvergenceTest *theTest ;

    double tolerance ;  // line search tolerance 
    
    Vector *r0, *x0, *x, *xOld;

};

#endif


