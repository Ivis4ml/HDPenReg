/*--------------------------------------------------------------------*/
/*     Copyright (C) 2013-2013  Serge Iovleff, Quentin Grimonprez

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place,
    Suite 330,
    Boston, MA 02111-1307
    USA

    Contact : quentin.grimonprez@inria.fr
*/

/*
 * Project:  MPAGenomics::
 * created on: 17 dec. 2013
 * Author:   Quentin Grimonprez
 **/

/** @file LogisticFusedLasso.cpp
 *  @brief In this file .
 **/

#include "LogisticFusedLasso.h"

namespace HD
{
  /*default constructor*/
  LogisticFusedLasso::LogisticFusedLasso()
      : PenalizedModels<LogisticFusedLasso>()
  {
    STK::CVectorX Xty(0);
    // creation of the fusedlasso penalty
    LogisticFusedLassoPenalty* p_penalty = new LogisticFusedLassoPenalty();
    p_penalty_ = p_penalty;
    //creation functor for CG
    FusedLassoMultiplicator mult(0, p_penalty_->p_mainDiagonal(), p_penalty_->p_offDiagonal(), p_penalty_->p_sigma2());

    mult_ = mult;
    //create CG
    STK::CG<FusedLassoMultiplicator,STK::CVectorX, InitFunctor>* p_gcsolver = new STK::CG<FusedLassoMultiplicator,STK::CVectorX,InitFunctor>(mult_,Xty);

    p_gcsolver_ = p_gcsolver;

    //create solver for fused lasso
    LogisticFusedLassoSolver* p_fusedlassosolver = new LogisticFusedLassoSolver;
    p_fusedlassosolver->setPenalty(p_penalty_);
    p_fusedlassosolver->setSolver(p_gcsolver_);

    //add solver to lasso
    p_solver_ = p_fusedlassosolver;

    mult_.p_data_ = p_currentData();
  }


  /*
   * constructor
   * @param p_data pointer to the data
   * @param p_y pointer to the response
   * @param lambda1 value of parameter associated to the l1 penalty
   * @param lambda2 value of parameter associated to the l1 penalty of successive coefficients
   * @param threshold threshold for setting coefficient to 0
   * @param epsCG epsilon for CG convergence
   */
  LogisticFusedLasso::LogisticFusedLasso(STK::CArrayXX const* p_data, STK::CVectorX const* p_y, STK::Real lambda1, STK::Real lambda2, STK::Real threshold, STK::Real epsCG)
  : PenalizedModels<LogisticFusedLasso>(p_data, p_y)
  {
    STK::CVectorX Xty(nbVariable());
    Xty = p_data_->transpose() * *p_y;

    STK::CVectorX beta0(Xty.sizeRows());
    for(int i = 1; i <= Xty.sizeRows();i++)
      beta0[i] = Xty[i]/((*p_data).col(i).norm2());

    beta_ = beta0;

    // creation fusedlasso penalty
    LogisticFusedLassoPenalty* p_penalty = new LogisticFusedLassoPenalty(lambda1, lambda2, threshold);
    p_penalty_ = p_penalty;
    //creation functor for CG
    FusedLassoMultiplicator mult(0, p_penalty_->p_mainDiagonal(), p_penalty_->p_offDiagonal(), p_penalty_->p_sigma2());

    mult_ = mult;
    //create CG
    STK::CG<FusedLassoMultiplicator,STK::CVectorX, InitFunctor>* p_gcsolver = new STK::CG<FusedLassoMultiplicator,STK::CVectorX,InitFunctor>(mult_, Xty,0 ,epsCG);

    p_gcsolver_=p_gcsolver;
    //create solver for fused lasso
    LogisticFusedLassoSolver* p_fusedlassosolver = new LogisticFusedLassoSolver(p_data_, beta_, p_y_, p_gcsolver_, p_penalty_, threshold);

    //add the penalty
    p_fusedlassosolver->setPenalty(p_penalty_);

    //add solver to lasso
    p_solver_ = p_fusedlassosolver;

    InitFunctor init(p_solver_->p_currentBeta());
    init_ = init;
    (p_solver_->p_solver())->setInitFunctor(&init_);

    mult_.p_data_ = p_currentData();
  }


  /* destructor*/
  LogisticFusedLasso::~LogisticFusedLasso()
  {
    if(p_gcsolver_) delete p_gcsolver_;
  }

  /*initialize the containers of all subclasses*/
  void LogisticFusedLasso::initializeModel()
  {
    //computation of the initial solution : beta0
    STK::CVectorX Xty(nbVariable());
    Xty = p_data_->transpose() * (*p_y_);

    STK::CVectorX beta0(Xty.sizeRows());
    for(int i = 1; i <= Xty.sizeRows();i++)
      beta0[i] = Xty[i]/((*p_data_).col(i).norm2());

    //add response to penalty
    p_penalty_->setPy(p_y_);

    beta_ = beta0;

    //set the parameters of the FusedLassoSolver
    p_solver_->setData(p_data_);
    p_solver_->setY(p_penalty_->p_z());
    p_solver_->setBeta(beta_);
    p_solver_->initializeSolver();

    //initialization functor for the CG
    InitFunctor init(p_solver_->p_currentBeta());
    init_ = init;
    p_gcsolver_->setInitFunctor(&init_);
    //add currentData to penalty
    p_penalty_->setPcurrentData(p_solver_->p_currentData());
  }
}
