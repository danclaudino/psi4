#ifndef three_index_H
#define three_index_H

#include <psi4-dec.h>
#include <psiconfig.h>
#include <libmints/matrix.h>

namespace psi {

class PSIO;
class BasisSet;
class Matrix;
class Vector;
class IntVector;
class GridBlock;
class PseudospectralInt;

class FittingMetric {

protected:
    /// Pointer to the auxiliary basis set
    shared_ptr<BasisSet> aux_;
    /// Pointer to the poisson basis set
    shared_ptr<BasisSet> pois_;
    /// Is the metric poisson?
    bool is_poisson_;

    /// The fitting metric or symmetric inverse
    shared_ptr<Matrix> metric_;
    /// The indices (per irrep) of pivots
    shared_ptr<IntVector> pivots_;
    /// The indices (per irrep) of reverse pivots 
    shared_ptr<IntVector> rev_pivots_;

    /// The fitting algorithm selected 
    std::string algorithm_;
    /// Is the metric inverted or just a J matrix?
    bool is_inverted_;

    /// Fully pivot the fitting metric
    void pivot();

public:

    /// Default constructor, for python
    FittingMetric();
    /// DF Fitting Metric
    FittingMetric(shared_ptr<BasisSet> aux);
    /// Poisson Fitting Metric
    FittingMetric(shared_ptr<BasisSet> aux, shared_ptr<BasisSet> pois);

    /// Destructor
    ~FittingMetric();
   
    /// What algorithm to use for symmetric inverse? 
    std::string get_algorithm() const {return algorithm_; }
    /// Are poisson functions used?
    bool is_poisson() const {return is_poisson_; }
    /// Is the metric inverted? 
    bool is_inverted() const {return is_inverted_; }

    /// The fitting metric or symmetric inverse
    shared_ptr<Matrix> get_metric() const {return metric_; }
    /// The vector of pivots (for stability) (pivoted->global)
    shared_ptr<IntVector> get_pivots() const {return pivots_; }
    /// The vector of back pivots (for stability) (global->pivoted)
    shared_ptr<IntVector> get_reverse_pivots() const {return rev_pivots_; }

    /// The gaussian fitting basis
    shared_ptr<BasisSet> get_auxiliary_basis() const {return aux_; }
    /// The poisson fitting basis
    shared_ptr<BasisSet> get_poisson_basis() const {return pois_; }

    /// Build the raw fitting metric (sets up indices to canonical)
    void form_fitting_metric();
    /// Build the Cholesky half inverse metric (calls form_fitting_metric)
    void form_cholesky_inverse();
    /// Build the QR half inverse metric (calls form_fitting_metric)
    void form_QR_inverse(double tol = 1.0E-10);
    /// Build the eigendecomposed half inverse metric (calls form_fitting_metric)
    void form_eig_inverse(double tol = 1.0E-10);
    /// Build the full inverse metric. NOT RECOMMENDED: Numerical stability (calls form_fitting_metric)
    void form_full_inverse();
    /// Build the full metric's Cholesky factor. RECOMMENDED: Numerical stability
    void form_cholesky_factor();
};

class PseudoGrid {

protected:
    /// The grid (in a Jeff Bridges voice)
    shared_ptr<GridBlock> grid_;
    /// Name (ie: cc-pVQZ-ultrafine)
    std::string name_;
    /// The molecule the grid is build on
    shared_ptr<Molecule> molecule_;
    
public:
    /// Construct a grid object from a molecule and name, then parse (or maybe autogen)
    PseudoGrid(shared_ptr<Molecule> mol, const std::string& name);
    /// Destructor, frees grid memory if created
    ~PseudoGrid();

    /// The GridBlock associated with this grid
    shared_ptr<GridBlock> getBlock() const { return grid_; } 
    /// Parse grid given full file path to G94-style grid file
    void parse(const std::string& file);
    
};

class SchwarzSieve {

protected:

    // The schwarz cutoff 
    double schwarz_;

    // Basis set for this schwarz
    shared_ptr<BasisSet> basis_;  

    // Is the sieve initialized
    bool initialized_;
 
    // number of significant shell pairs 
    unsigned long int nshell_pairs_;
    // number of significant function pairs
    unsigned long int nfun_pairs_;

    double max_global_val_;
    int* schwarz_shells_;
    int* schwarz_funs_;

    long int* schwarz_shells_reverse_;
    long int* schwarz_funs_reverse_;

    double* schwarz_shell_vals_;
    double* schwarz_fun_vals_;

    void form_schwarz_ints();

public:
    SchwarzSieve(shared_ptr<BasisSet>, double cutoff);
    virtual ~SchwarzSieve();

    void form_schwarz_sieve(double cutoff);
    // Sizes of the significant bra/ket pairs
    unsigned long int get_nshell_pairs() const { return nshell_pairs_; }
    unsigned long int get_nfun_pairs() const { return nfun_pairs_; }
    // I_global = arr[2*I_local], J_global = arr[2*I_local + 1]
    // These are only defined up to nshell_pairs_ and nfun_pairs_, respectively
    int* get_schwarz_shells() const { return schwarz_shells_; }
    int* get_schwarz_funs() const { return schwarz_funs_; }
    // Canonical compound indexing, -1 if not present
    long int* get_schwarz_shells_reverse() const { return schwarz_shells_reverse_; }
    long int* get_schwarz_funs_reverse() const { return schwarz_funs_reverse_; }

};

// Disk-based tensor random access TensorChunk iterator
class TensorChunk {
    
    protected:
        // Name
        std::string name_;
        // PSIO 
        shared_ptr<PSIO> psio_;
        // PSIO unit number
        unsigned int unit_;

        // memory (doubles)
        unsigned long int memory_;
        // tensor size (doubles)
        unsigned long int tensor_size_;
        // max_rows permitted by memory
        int max_rows_;  
        // min_rows permitted by user (for instance, if all a are required for any i in an ia compound slow indeX)
        int min_rows_;  
        // slow index total size
        int slow_size_; 
        // fast index total size
        int fast_size_; 
 
        // Is a block read in?
        bool touched_;

        // Tensor chunk
        shared_ptr<Matrix> chunk_;
    
        // Block starts
        std::vector<int> block_starts_;
        // Block sizes
        std::vector<int> block_sizes_;
        // Current block
        int block_;

    public:
        // Disk algorithm constructor
        TensorChunk(shared_ptr<PSIO> psio,       // PSIO for this tensor 
                        unsigned int unit,       // unit for this tensor (must be opened and closed by the driver)
                        const std::string& name, // entry name of this tensor
                        int slow_size,           // size of the slow index
                        int fast_size,           // size of the fast index
                        unsigned long int memory,// memory available in double
                        int min_rows = 1         // minimum rows allowed (i.e. the the grain size)
                        );
        ~TensorChunk();

        // Interaction operators:
        // Number of blocks required for a complete traverse of the tensor
        int nblock() const { return block_starts_.size(); }
        // Pointer to the chunk 
        shared_ptr<Matrix> chunk() const { return chunk_; }
        // Pointer to the chunk pointer 
        double** chunk_pointer() const { return chunk_->pointer(); }
        // read in the chunk corresponding to block_number
        // If you need read-only access, and the tensor is small enough to 
        // fit in core under the memory provided (nblock is 1), setting
        // keep_if_cached = true will forgo IO on repreated calls to this operation
        // (e.g., core and disk algorithms are semi-similar) 
        void read_block(int block_number, bool keep_if_cached = false);
        // The starting slow index of the current block 
        int block_start() const { return block_starts_[block_]; }
        // The slow index size of the current block 
        int block_size() const { return block_sizes_[block_]; }
        // The current block index
        int block() const { return block_; }

        // PSIO unit number
        unsigned int unit() const {return unit_;}
        // Name (corresponds to entry name) 
        std::string name() const { return name_; }
        // Slow index total size
        int slow_size() const { return slow_size_; }        
        // Fast index total size
        int fast_size() const { return fast_size_; }        
        // Memory allowed for this iterator 
        unsigned long int memory() const { return memory_; }
        // Maximum size of slow index in chunk  
        int max_rows() const { return max_rows_; } 
        // Minimum size of slow index in chunk (usually 1, unless slow index is compound)
        int min_rows() const { return min_rows_; } 
};


class DFTensor {
private:

    // Should the integral file be deleted?
    int keep_;

protected:

    // The fitting metric (if still needed)
    shared_ptr<FittingMetric> metric_;
    // The algorithm to fit with
    std::string fitting_algorithm_;
    // The fitting condition to go for
    double fitting_condition_;

    // The primary basis
    shared_ptr<BasisSet> primary_;
    // The auxiliary basis
    shared_ptr<BasisSet> auxiliary_;

    // The cutoff for the schwarz sieve
    double schwarz_cutoff_;

    // The active occupied C matrix
    shared_ptr<Matrix> Co_;
    // The active virtual C matrix
    shared_ptr<Matrix> Cv_;

    // The amount of available memory for tensor formation, in doubles
    unsigned long int memory_;

    // The PSIO object
    shared_ptr<PSIO> psio_;
    
    // The striping decision (Qia if true, iaQ if false)
    bool Qia_striping_;

    // Number of occupied orbitals
    int nocc_;
    // Number of virtual orbitals
    int nvir_;
    // Number of atomic orbitals
    int nao_;
    // Number of fitting functions
    int naux_;

    // common startup stuff
    void common_init();     
    // forms the unfitted MO integrals
    void form_Aia(bool do_all);
    // applies the fitting (assumes the metric is formed)
    void apply_fitting(const std::string& entry);

public:
    DFTensor(shared_ptr<PSIO>, shared_ptr<BasisSet> primary, shared_ptr<BasisSet> aux);
    DFTensor(shared_ptr<PSIO>, shared_ptr<BasisSet> primary, shared_ptr<BasisSet> aux, bool);
    virtual ~DFTensor();

    // Form all MO integrals 
    void form_MO_integrals(unsigned long int memory_doubles, shared_ptr<Matrix> Co, shared_ptr<Matrix> Cv, bool Qia_striping, const std::string& fitting_algorithm,
        double condition, double schwarz);
    // Form only OV integrals 
    void form_OV_integrals(unsigned long int memory_doubles, shared_ptr<Matrix> Co, shared_ptr<Matrix> Cv, bool Qia_striping, const std::string& fitting_algorithm,
        double condition, double schwarz);
    
    // Iterators to the various blocks of the MO DF integrals, 
    // memory in doubles  
    shared_ptr<TensorChunk> get_oo_iterator(unsigned long int memory);
    shared_ptr<TensorChunk> get_vv_iterator(unsigned long int memory);
    shared_ptr<TensorChunk> get_ov_iterator(unsigned long int memory);

    // Number of ao basis functions in this DF tensor
    int nao() const { return nao_; }   
    // Number of auxiliary basis functions in this DF tensor
    int naux() const { return naux_; }   
    // Number of occupied orbtials in this DF tensor
    int nocc() const { return nocc_; }   
    // Number of virtual orbitals in this DF tensor
    int nvir() const { return nvir_; }   

};

class PSTensor {
    
protected:
    // Pointer to the primary basis
    shared_ptr<BasisSet> primary_;
    // Pointer to the dealias basis 
    shared_ptr<BasisSet> dealias_;
    // Pointer to the pseudospectral grid
    shared_ptr<PseudoGrid> grid_;
    // Number of occupied orbitals
    int nocc_;
    // Number of virtual orbitals
    int nvir_;
    // Number of atomic orbitals
    int nao_;
    // Number of fitting functions (grid points)
    int naux_;
    // The psio object (for disk-based)
    shared_ptr<PSIO> psio_;

    // The cutoff for the schwarz sieve
    double schwarz_cutoff_;
    // The striping decision (Qia if true, iaQ if false)
    bool Qia_striping_;
    // Memory in doubles
    unsigned long int memory_;

    // The active occupied C matrix
    shared_ptr<Matrix> Co_;
    // The active virtual C matrix
    shared_ptr<Matrix> Cv_;

    // Helper routines (numerically ninja)    
    shared_ptr<Matrix> form_X_AO();
    shared_ptr<Matrix> form_X_dealias();
    shared_ptr<Matrix> form_S();
    shared_ptr<Matrix> form_S_dealias();
    shared_ptr<Matrix> form_Q_AO();
    void common_init();
    // forms the unfitted MO integrals
    void form_Aia(bool do_all);
    void restripe(const std::string& entry);
public:
    PSTensor(shared_ptr<PSIO>, shared_ptr<BasisSet> primary, shared_ptr<BasisSet> aux, shared_ptr<PseudoGrid> grid);
    virtual ~PSTensor();

    // Form Q_i^P (Fitted matrix)
    shared_ptr<Matrix> form_Q(shared_ptr<Matrix> Cocc);
    // Form Q_a^P (Collocation matrix)
    shared_ptr<Matrix> form_X(shared_ptr<Matrix> Cvir);

    // Form all MO integrals 
    void form_MO_integrals(unsigned long int memory_doubles, shared_ptr<Matrix> Co, shared_ptr<Matrix> Cv, bool Qia_striping, double schwarz);
    // Form only OV integrals 
    void form_OV_integrals(unsigned long int memory_doubles, shared_ptr<Matrix> Co, shared_ptr<Matrix> Cv, bool Qia_striping, double schwarz);
    
    // Iterators to the various blocks of the MO DF integrals, 
    // memory in doubles  
    shared_ptr<TensorChunk> get_oo_iterator(unsigned long int memory);
    shared_ptr<TensorChunk> get_vv_iterator(unsigned long int memory);
    shared_ptr<TensorChunk> get_ov_iterator(unsigned long int memory);

    // Number of ao basis functions in this PS tensor
    int nao() const { return nao_; }   
    // Number of auxiliary basis functions (grid points) in this PS tensor
    int naux() const { return naux_; }   
    // Number of occupied orbtials in this PS tensor
    int nocc() const { return nocc_; }   
    // Number of virtual orbitals in this PS tensor
    int nvir() const { return nvir_; }   

};

class DirectPSTensor : public PSTensor {

protected:

    // Memory (doubles)
    unsigned long int memory_;
    // Max rows 
    int max_rows_;
    // Block starts
    std::vector<int> block_starts_;
    // Block sizes
    std::vector<int> block_sizes_;
    // Current block
    int block_;

    // (A|mn) slices
    shared_ptr<Matrix> Amn_;
    // (A|mi) slices 
    shared_ptr<Matrix> Ami_; 
    // (A|ia)
    shared_ptr<Matrix> Aia_;

    // Schwarz sieve object
    shared_ptr<SchwarzSieve> schwarz_;

    // Pseudospectral integral objects (threaded)
    std::vector<shared_ptr<PseudospectralInt> > ints_; 

public:
    DirectPSTensor(shared_ptr<PSIO>, shared_ptr<BasisSet> primary, shared_ptr<BasisSet> aux, shared_ptr<PseudoGrid> grid);
    virtual ~DirectPSTensor();

    // Initialization routine for (A|ia) striping (others to be added later)
    void initialize_OV_integrals(shared_ptr<Matrix> Cocc, shared_ptr<Matrix> Cvir, unsigned long int memory, double schwarz_cutoff);

    // Interaction operators:
    // Number of blocks required for a complete traverse of the tensor
    int nblock() const { return block_starts_.size(); }
    // Pointer to the chunk (P| 
    shared_ptr<Matrix> chunk() const { return Aia_; }
    // Pointer to the chunk pointer 
    double** chunk_pointer() const { return Aia_->pointer(); }
    // Compute the desired block number 
    void compute_block(int block_number);
    // The starting slow index of the current block 
    int block_start() const { return block_starts_[block_]; }
    // The slow index size of the current block 
    int block_size() const { return block_sizes_[block_]; }
    // The current block index
    int block() const { return block_; }

    // Memory allowed for this iterator 
    unsigned long int memory() const { return memory_; }
    // Maximum size of slow index in chunk  
    int max_rows() const { return max_rows_; } 

};
/**
class Pseudospectral {

protected:
    shared_ptr<BasisSet> primary_;
    shared_ptr<BasisSet> dealias_;
    shared_ptr<PseudoGrid> grid_;
    int npoints_;
    shared_ptr<PSIO> psio_;
public:
    Pseudospectral(shared_ptr<PSIO>, shared_ptr<BasisSet> primary, shared_ptr<BasisSet> dealias, shared_ptr<PseudoGrid> grid);
    ~Pseudospectral();
    
    shared_ptr<Matrix> form_X();
    shared_ptr<Matrix> form_X_dealias();
    shared_ptr<Matrix> form_S();
    shared_ptr<Matrix> form_S_dealias();
    shared_ptr<Matrix> form_Q();
    shared_ptr<Matrix> form_A();
    void form_A_disk();
    int npoints() const { return npoints_; }

    shared_ptr<Matrix> form_I(); 

};
**/

// Denominator Factorizations (MP2-like for now)
class Denominator {

protected:
    // Denominator (w in rows, ia in column)
    shared_ptr<Matrix> denominator_;

    // Pointer to active occupied orbital eigenvalues
    shared_ptr<Vector> eps_occ_;
    // Pointer to active virtual orbital eigenvalues
    shared_ptr<Vector> eps_vir_;
    // Number of vectors required to obtain given accuracy
    int nvector_;   
    // Maximum error norm allowed in denominator
    double delta_; 

    virtual void decompose() = 0; 
public:
    Denominator(shared_ptr<Vector> eps_occ_, shared_ptr<Vector> eps_vir, double delta);
    virtual ~Denominator();

    double delta() const { return delta_; }
    int nvector() const { return nvector_; }
    virtual void debug();
    shared_ptr<Matrix> denominator() const { return denominator_; }

};

class LaplaceDenominator : public Denominator {

protected:
    // Fully split denominator (w in rows, i in columns) 
    shared_ptr<Matrix> denominator_occ_;
    // Fully split denominator (w in rows, a in columns) 
    shared_ptr<Matrix> denominator_vir_;

    void decompose();
public:
    LaplaceDenominator(shared_ptr<Vector> eps_occ_, shared_ptr<Vector> eps_vir, double delta);
    ~LaplaceDenominator();
    void debug();
    shared_ptr<Matrix> denominator_occ() const { return denominator_occ_; }
    shared_ptr<Matrix> denominator_vir() const { return denominator_vir_; }
    
};

class CholeskyDenominator : public Denominator {

protected:
    double degeneracy_multiplier_;    
    void decompose();
    static bool criteria(std::pair<int, double> a, std::pair<int, double> b);
public:
    CholeskyDenominator(shared_ptr<Vector> eps_occ_, shared_ptr<Vector> eps_vir, double delta, double degeneracy_multiplier = 100.0);
    ~CholeskyDenominator();
    void debug();

};

}
#endif
