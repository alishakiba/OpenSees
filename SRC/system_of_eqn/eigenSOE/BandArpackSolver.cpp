// File: ~/system_of_eqn/eigenSOE/BandArpackSolver.C
//
// Written: Jun Peng
// Created: Feb. 11, 1999
// Revision: A
//
// Description: This file contains the class definition for 
// BandArpackSolver. It solves the BandArpackSOE object by calling
// Arpack routines.

#include <BandArpackSolver.h>
#include <BandArpackSOE.h>
#include <f2c.h>
#include <math.h>
#include <stdio.h>
#include <AnalysisModel.h>
#include <DOF_GrpIter.h>
#include <DOF_Group.h>
#include <FE_EleIter.h>
#include <FE_Element.h>
#include <Integrator.h>

BandArpackSolver::BandArpackSolver(int numE)
:EigenSolver(EigenSOLVER_TAGS_BandArpackSolver),
 theNev(numE), iPiv(0), iPivSize(0), eigenV(0)
{
  // do nothing here.    
}


BandArpackSolver::~BandArpackSolver()
{
    if (iPiv != 0)
	delete [] iPiv;
    if (value != 0)
        delete [] value;
    if (eigenvector !=0)
        delete [] eigenvector;
    if (eigenV !=0)
        delete eigenV;
}

#ifdef _WIN32

extern "C" int _stdcall DGBSV(int *N, int *KL, int *KU, int *NRHS, double *A, 
			      int *LDA, int *iPiv, double *B, int *LDB, 
			      int *INFO);

extern "C" int _stdcall DGBTRF(int *M, int *N, int *KL, int *KU, double *A, int *LDA, 
			       int *iPiv, int *INFO);

extern "C" int _stdcall DGBTRS(char *TRANS, unsigned int *sizeC, 
			       int *N, int *KL, int *KU, int *NRHS,
			       double *A, int *LDA, int *iPiv, 
			       double *B, int *LDB, int *INFO);

extern "C" int _stdcall DSAUPD(int *ido, char* bmat, unsigned int *, 
			       int *n, char *which, unsigned int *,
			       int *nev, 
			       double *tol, double *resid, int *ncv, double *v, 
			       int *ldv,
			       int *iparam, int *ipntr, double *workd, double *workl,
			       int *lworkl, int *info);

extern "C" int _stdcall DSEUPD(bool *rvec, char *howmny, unsigned int *,
			       logical *select, double *d, double *z,
			       int *ldz, double *sigma, char *bmat, unsigned int *,
			       int 	*n, char *which, unsigned int *,
			       int *nev, double *tol, double *resid, int *ncv, 
			       double *v,
			       int *ldv, int *iparam, int *ipntr, double *workd, 
			       double *workl, int *lworkl, int *info);

#else

extern "C" int dgbsv_(int *N, int *KL, int *KU, int *NRHS, double *A, int *LDA, 
		      int *iPiv, double *B, int *LDB, int *INFO);

extern "C" int dgbtrf_(int *M, int *N, int *KL, int *KU, double *A, int *LDA, 
		       int *iPiv, int *INFO);
		       
extern "C" int dgbtrs_(char *TRANS, int *N, int *KL, int *KU, int *NRHS, 
		       double *A, int *LDA, int *iPiv, double *B, int *LDB, 
		       int *INFO);

extern "C" int dsaupd_(int *ido, char* bmat, int *n, char *which, int *nev, 
		       double *tol, double *resid, int *ncv, double *v, int *ldv,
		       int *iparam, int *ipntr, double *workd, double *workl,
		       int *lworkl, int *info);

extern "C" int dseupd_(bool *rvec, char *howmny, logical *select, double *d, double *z,
		       int *ldz, double *sigma, char *bmat, int *n, char *which,
		       int *nev, double *tol, double *resid, int *ncv, double *v,
		       int *ldv, int *iparam, int *ipntr, double *workd, 
		       double *workl, int *lworkl, int *info);

#endif




int
BandArpackSolver::solve(void)
{

    if (theSOE == 0) {
	cerr << "WARNING BandGenLinLapackSolver::solve(void)- ";
	cerr << " No LinearSOE object has been set\n";
	return -1;
    }

    int n = theSOE->size;    
    
    // check iPiv is large enough
    if (iPivSize < n) {
	cerr << "WARNING BandGenLinLapackSolver::solve(void)- ";
	cerr << " iPiv not large enough - has setSize() been called?\n";
	return -1;
    }	    

    // set some variables
    int kl = theSOE->numSubD;
    int ku = theSOE->numSuperD;
    int ldA = 2*kl + ku +1;
    int nrhs = 1;
    int ldB = n;
    int info;
    double *Aptr = theSOE->A;
    int    *iPIV = iPiv;

    int nev = theNev;
    int ncv = getNCV(n, nev);

    // set up the space for ARPACK functions.
    // this is done each time method is called!! .. this needs to be cleaned up
    int ldv = n;
    int lworkl = ncv*ncv + 8*ncv;
    double *v = new double[ldv * ncv];
    double *workl = new double[lworkl + 1];
    double *workd = new double[3 * n + 1];
    double *d = new double[nev];
    double *z= new double[n * nev];
    double *resid = new double[n];
    int *iparam = new int[11];
    int *ipntr = new int[11];
    logical *select = new logical[ncv];

    static char which[3]; strcpy(which, "LM");
    char bmat = 'G';
    char howmy = 'A';

    // some more variables
    int maxitr, mode;
    double tol = 0.0;
    info = 0;
    maxitr = 1000;
    mode = 3;

    iparam[0] = 1;
    iparam[2] = maxitr;
    iparam[6] = mode; 

    bool rvec = true;

    int ido = 0;

    int ierr = 0;

    // Do the factorization of Matrix (A-dM) here.
#ifdef _WIN32
    DGBTRF(&n, &n, &kl, &ku, Aptr, &ldA, iPiv, &ierr); 
#else
    dgbtrf_(&n, &n, &kl, &ku, Aptr, &ldA, iPiv, &ierr); 
#endif

    if ( ierr != 0 ) {
       cerr << " BandArpackSolver::Error in dgbtrf_ " << endl;
       return -1;
    }

    while (1) { 

#ifdef _WIN32
      unsigned int sizeWhich =2;
      unsigned int sizeBmat =1;
      unsigned int sizeHowmany =1;
	  unsigned int sizeOne = 1;
      DSAUPD(&ido, &bmat, &sizeBmat, &n, which, &sizeWhich, &nev, &tol, resid, 
	     &ncv, v, &ldv,
	     iparam, ipntr, workd, workl, &lworkl, &info);
#else
      dsaupd_(&ido, &bmat, &n, which, &nev, &tol, resid, &ncv, v, &ldv,
	      iparam, ipntr, workd, workl, &lworkl, &info);
#endif

      if (ido == -1) {

	  myMv(n, &workd[ipntr[0]-1], &workd[ipntr[1]-1]); 
#ifdef _WIN32
	  DGBTRS("N", &sizeOne, &n, &kl, &ku, &nrhs, Aptr, &ldA, iPIV, 
		 &workd[ipntr[1] - 1], &ldB, &ierr);
#else
	  dgbtrs_("N", &n, &kl, &ku, &nrhs, Aptr, &ldA, iPIV, 
		  &workd[ipntr[1] - 1], &ldB, &ierr);
#endif

	  if (ierr != 0) {
	      cerr << "BandArpackSolver::Error with dgbtrs_ 1" <<endl;
	      exit(0);
	  }
	  continue;
      } else if (ido == 1) {

	//          double ratio = 1.0;
	  myCopy(n, &workd[ipntr[2]-1], &workd[ipntr[1]-1]);

#ifdef _WIN32
	  DGBTRS("N", &sizeOne, &n, &kl, &ku, &nrhs, Aptr, &ldA, iPIV, 
		 &workd[ipntr[1] - 1], &ldB, &ierr);
#else
	  dgbtrs_("N", &n, &kl, &ku, &nrhs, Aptr, &ldA, iPIV, 
		  &workd[ipntr[1] - 1], &ldB, &ierr);
#endif

	  if (ierr != 0) {
	      cerr << "BandArpackSolver::Error with dgbtrs_ 2" <<endl;
	      exit(0);
	  }
	  continue;
      } else if (ido == 2) {     

	  myMv(n, &workd[ipntr[0]-1], &workd[ipntr[1]-1]);
	  continue;
      }

      break;
    }

    if (info < 0) {
      cerr << "BandArpackSolver::Error with _saupd info = " << info << endl;
      switch(info) {

         case -1: 
	   cerr << "N must be positive.\n";
	   break;
         case -2: 
	   cerr << "NEV must be positive.\n";
	   break;
         case -3: 
	   cerr << "NCV must be greater than NEV and less than or equal to N.\n";
	   break;
         case -4:
	   cerr << "The maximum number of Arnoldi update iterations allowed";
	   break;
         case -5: 
	   cerr << "WHICH must be one of 'LM', 'SM', 'LA', 'SA' or 'BE'.\n";
	   break;
         case -6: 
	   cerr << "BMAT must be one of 'I' or 'G'.\n";
	   break;
         case -7: 
	   cerr << "Length of private work array WORKL is not sufficient.\n";
	   break;
         case -8: 
	   cerr << "Error return from trid. eigenvalue calculation";
	   cerr << "Informatinal error from LAPACK routine dsteqr.\n";
	   break;
         case -9: 
	   cerr << "Starting vector is zero.\n";
	   break;
         case -10: 
	   cerr << "IPARAM(7) must be 1,2,3,4,5.\n";
	   break;
         case -11: 
	   cerr << "IPARAM(7) = 1 and BMAT = 'G' are incompatable.\n";
	   break;
         case -12: 
	   cerr << "IPARAM(1) must be equal to 0 or 1.\n";
	   break;
         case -13:
	   cerr << "NEV and WHICH = 'BE' are incompatable.\n";
	   break;
         case -9999:
	   cerr << "Could not build an Arnoldi factorization.";
	   cerr << "IPARAM(5) returns the size of the current Arnoldi\n";
	   cerr << "factorization. The user is advised to check that";
	   cerr << "enough workspace and array storage has been allocated.\n";
	   break;
         default:
	   cerr << "unrecognised return value\n";
      }

      // clean up the memory
      delete [] workl;
      delete [] workd;
      delete [] resid;
      delete [] iparam;
      delete [] v;
      delete [] select;
      delete [] ipntr;
      delete [] d;
      delete [] z;

      value = 0;
      eigenvector = 0;
		
      return info;
    } else {
        if (info == 1) {
	  cerr << "BandArpackSolver::Maximum number of iteration reached." << endl;
	} else if (info == 3) {
	  cerr << "BandArpackSolver::No Shifts could be applied during implicit,";
	  cerr << "Arnoldi update, try increasing NCV." << endl;
	}

	double sigma = theSOE->shift;
	if (iparam[4] > 0) {
	    rvec = true;
	    n = theSOE->size;    
	    ldv = n;

#ifdef _WIN32
	    unsigned int sizeWhich =2;
	    unsigned int sizeBmat =1;
	    unsigned int sizeHowmany =1;
	    DSEUPD(&rvec, &howmy, &sizeHowmany, select, d, z, &ldv, &sigma, &bmat,
		   &sizeBmat, &n, which, &sizeWhich,
		   &nev, &tol, resid, &ncv, v, &ldv, iparam, ipntr, workd,
		   workl, &lworkl, &info);
#else
	    dseupd_(&rvec, &howmy, select, d, z, &ldv, &sigma, &bmat, &n, which,
		    &nev, &tol, resid, &ncv, v, &ldv, iparam, ipntr, workd,
		    workl, &lworkl, &info);
#endif
	    if (info != 0) {
	        cerr << "BandArpackSolver::Error with dseupd_" << info;
		switch(info) {

		case -1: 
		   cerr << " N must be positive.\n";
		   break;
		case -2: 
		   cerr << " NEV must be positive.\n";
		   break;
		case -3: 
		   cerr << " NCV must be greater than NEV and less than or equal to N.\n";
		   break;
		case -5: 
		   cerr << " WHICH must be one of 'LM', 'SM', 'LA', 'SA' or 'BE'.\n";
		   break;
		case -6: 
                   cerr << " BMAT must be one of 'I' or 'G'.\n";
		   break;
		case -7: 
		   cerr << " Length of private work WORKL array is not sufficient.\n";
		   break;
		case -8: 
                   cerr << " Error return from trid. eigenvalue calculation";
		   cerr << "Information error from LAPACK routine dsteqr.\n";
		   break;
		case -9: 
                   cerr << " Starting vector is zero.\n";
		   break;
		case -10: 
                   cerr << " IPARAM(7) must be 1,2,3,4,5.\n";
		   break;
		case -11: 
                   cerr << " IPARAM(7) = 1 and BMAT = 'G' are incompatibl\n";
		   break;
		case -12: 
		   cerr << " NEV and WHICH = 'BE' are incompatible.\n";
		   break;
		case -14: 
		   cerr << " DSAUPD did not find any eigenvalues to sufficient accuracy.\n";
		   break;
		case -15: 
		   cerr << " HOWMNY must be one of 'A' or 'S' if RVEC = .true.\n";
		   break;
		case -16: 
		   cerr << " HOWMNY = 'S' not yet implemented\n";
		   break;
		default:
		  ;
		}

		// clean up the memory
		delete [] workl;
		delete [] workd;
		delete [] resid;
		delete [] iparam;
		delete [] v;
		delete [] select;
		delete [] ipntr;
		delete [] d;
		delete [] z;

		value = 0;
		eigenvector = 0;

		return info;

	    }
	}
    }


    value = d;
    eigenvector = z;

    theSOE->factored = true;

    // clean up the memory
    delete [] workl;
    delete [] workd;
    delete [] resid;
    delete [] iparam;
    delete [] v;
    delete [] select;
    delete [] ipntr;
	delete [] d;
	delete [] z;

    return 0;
}


int 
BandArpackSolver::getNCV(int n, int nev)
{
    int result;
    if (2*nev > nev+8) {
        result = nev+8;
    } else {
        result = 2*nev;
    }

    if (result >= n) {
        result = n;
    }

    return result;
}


void
BandArpackSolver::myMv(int n, double *v, double *result)
{
    Vector x(v, n);
    Vector y(result,n);

    y.Zero();
    AnalysisModel *theAnalysisModel = theSOE->theModel;

    // loop over the FE_Elements
    FE_Element *elePtr;
    FE_EleIter &theEles = theAnalysisModel->getFEs();    
    while((elePtr = theEles()) != 0) {
        elePtr->zeroResidual();
	elePtr->addM_Force(x,1.0);
	const Vector &a = elePtr->getResidual(0);
	y.Assemble(a,elePtr->getID(),1.0);
	//      const Vector &a = elePtr->getM_Force(x,1.0);
	//      y.Assemble(a,elePtr->getID(),1.0);

    }

    // loop over the DOF_Groups
    DOF_Group *dofPtr;
    DOF_GrpIter &theDofs = theAnalysisModel->getDOFs();
    Integrator *theIntegrator = 0;
    while ((dofPtr = theDofs()) != 0) {
        dofPtr->zeroUnbalance();
	dofPtr->addM_Force(x,1.0);
	const Vector &a = dofPtr->getUnbalance(theIntegrator);
	y.Assemble(a,dofPtr->getID(),1.0);
    }
}

void
BandArpackSolver::myCopy(int n, double *v, double *result)
{
    for (int i=0; i<n; i++) {
        result[i] = v[i];
    }
}


int
BandArpackSolver::setEigenSOE(BandArpackSOE &theBandSOE)
{
    theSOE = &theBandSOE;
    return 0;
}


const Vector &
BandArpackSolver::getEigenvector(int mode)
{
    if (mode <= 0 || mode > theNev) {
        cerr << "BandArpackSOE::getEigenvector() - mode is out of range(1 - nev)";
	eigenV->Zero();
	return *eigenV;  
    }

    int size = theSOE->size;
    int index = (mode - 1) * size;

    if (eigenvector != 0) {
      for (int i=0; i<size; i++) {
	(*eigenV)[i] = eigenvector[index++];
      }	
    }
    else {
        cerr << "BandArpackSOE::getEigenvalue() - eigenvectors not yet determined";
	eigenV->Zero();
    }      

    //Vector *eigenV = new Vector(&vector[index], size);
    return *eigenV;  
}


double
BandArpackSolver::getEigenvalue(int mode)
{
    if (mode <= 0 || mode > theNev) {
        cerr << "BandArpackSOE::getEigenvalue() - mode is out of range(1 - nev)";
	return -1;
    }

    if (value != 0)
      return value[mode-1];
    else {
        cerr << "BandArpackSOE::getEigenvalue() - eigenvalues not yet determined";
	return -2;
    }      
}
  

int
BandArpackSolver::setSize()
{
    // if iPiv not big enough, free it and get one large enough
    int size = theSOE->size;    
    if (iPivSize < size) {
	if (iPiv != 0)
	    delete [] iPiv;
	
	iPiv = new int[size];
	if (iPiv == 0) {
	    cerr << "WARNING BandGenLinLapackSolver::setSize() ";
	    cerr << " - ran out of memory for iPiv of size ";
	    cerr << theSOE->size << endl;
	    return -1;
	} else
	    iPivSize = size;
    }
    
    if (eigenV == 0 || eigenV->Size() != size) {
	if (eigenV != 0)
	    delete eigenV;

	eigenV = new Vector(size);
	if (eigenV == 0 || eigenV->Size() != size) {
	    cerr << "WARNING BandGenLinLapackSolver::setSize() ";
	    cerr << " - ran out of memory for eigenVector of size ";
	    cerr << theSOE->size << endl;
	    return -2;	    
	}
    }
	    
    return 0;
}


int    
BandArpackSolver::sendSelf(int commitTag, Channel &theChannel)
{
    return 0;
}

int
BandArpackSolver::recvSelf(int commitTag, Channel &theChannel, 
			   FEM_ObjectBroker &theBroker)
{
    // nothing to do
    return 0;
}

 

