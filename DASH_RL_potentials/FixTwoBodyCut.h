#pragma once
#ifndef FIXTWOBODYCUT_H
#define FIXTWOBODYCUT_H

#include "FixPair.h"
#include "xml_func.h"
//! Make FixLJCut available to the pair base class in boost
class EvaluatorWrapper;
void export_FixTwoBodyCut();

//! Fix for truncated Lennard-Jones interactions
/*!
 * Fix to calculate Lennard-Jones interactions of particles. The LJ potential
 * is defined as
 * \f[
 * V(r_{ij}) = 4 \varepsilon \left[ \left(\frac{\sigma}{r_{ij}}\right)^{12} -
 *                               \left(\frac{\sigma}{r_{ij}}\right)^{6}\right],
 * \f]
 * where \f$ r \f$ is the distance between two particles and \f$ \varepsilon \f$
 * and \f$ \sigma \f$ are the two relevant parameters. The LJ pair interaction
 * is only calculated for particles closer than \$r_{\text{cut}}\$.
 *
 * From the potential, the force can be derived as
 * \f[
 * F(r_{ij}) = 24 \varepsilon \frac{1}{r} \left[ 
 *                          2 \left(\frac{\sigma}{r_{ij}}\right)^{12} -
 *                            \left(\frac{\sigma}{r_{ij}}\right)^{6}
 *                          \right].
 * \f]
 * If \f$F(r_{ij}) < 0\f$, then the force is attractive. Otherwise, it is
 * repulsive.
*/

extern const std::string TwoBodyCutType;
class FixTwoBodyCut : public FixPair {
    public:
        //! Constructor
        FixTwoBodyCut(SHARED(State), std::string handle);

        //! Compute forces
        void compute(int);

        //! Compute single point energy
        void singlePointEng(float *);

        //! Prepare Fix
        /*!
         * \returns Always returns True
         * 
         * This function needs to be called before simulation run.
         */
        bool prepareForRun();

        //! Run after simulation
        /*!
         * This function needs to be called after simulation run.
         */
        bool postRun();

        //! Create restart string
        /*!
         * \param format Format of the pair parameters.
         *
                  * \returns restart chunk string.
         */
        std::string restartChunk(std::string format);


        //! Add new type of atoms
        /*!
         * \param handle Not used
         *
         * This function adds a new particle type to the fix.
         */
        void addSpecies(std::string handle);

        //! Return list of cutoff values
        std::vector<float> getRCuts();
    public:
        void setEvalWrapper();
        void setEvalWrapperOrig();

        const std::string epsHandle; //!< Handle for parameter epsilon
        const std::string sigHandle; //!< Handle for parameter sigma
        const std::string rCutHandle; //!< Handle for parameter rCut
        const std::string sigma_RHandle;
        const std::string r_AHandle;
        const std::string alpha_AHandle;
        const std::string GHandle;
        const std::string r_GHandle;
        const std::string sigma_GHandle;
        std::vector<float> epsilons; //!< vector storing epsilon values
        std::vector<float> sigmas; //!< vector storing sigma values
        std::vector<float> rCuts; //!< vector storing cutoff distance values
        std::vector<float> sigma_Rs;
        std::vector<float> r_As;
        std::vector<float> alpha_As;
        std::vector<float> Gs;
        std::vector<float> r_Gs;
        std::vector<float> sigma_Gs;

        //EvaluatorLJ evaluator; //!< Evaluator for generic pair interactions

};

#endif
