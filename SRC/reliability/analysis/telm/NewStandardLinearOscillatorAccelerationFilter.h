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

// $Revision: 1.1 $
// $Date: 2008-02-29 19:43:53 $
// $Source: /usr/local/cvs/OpenSees/SRC/reliability/analysis/telm/NewStandardLinearOscillatorAccelerationFilter.h,v $

#ifndef NewStandardLinearOscillatorAccelerationFilter_h
#define NewStandardLinearOscillatorAccelerationFilter_h

#include <Filter.h>

class NewStandardLinearOscillatorAccelerationFilter : public Filter
{

public:
	NewStandardLinearOscillatorAccelerationFilter(int tag, double period, double dampingRatio, double dtpulse);
	~NewStandardLinearOscillatorAccelerationFilter();
	double getAmplitude(double time);
	double getMaxAmplitude();
	double getTimeOfMaxAmplitude();

	void Print(OPS_Stream &s, int flag =0);

protected:

private:
	double wn;
	double xi;
	double wd;
	double coefs;
	double coefc;
	double dtpulse;
};

#endif
