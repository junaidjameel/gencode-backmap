/**
 * Handling mapping of a gene object
 */
#ifndef geneMapper_hh
#define geneMapper_hh
#include "gxf.hh"
#include "gxfFeatureTree.hh"
class FeatureTransMap;
class PslMapping;
struct psl;

/* class that maps a gene to the new assemble */
class GeneMapper {
    private:
    const FeatureTransMap* fFeatureTransMap;  // object to performance mappings
    
    GxfFeatureVector getExons(const GxfFeatureNode* transcriptTree) const;
    PslMapping* mapTranscriptExons(const GxfFeatureNode* transcriptTree) const;

    void processTranscript(const GxfFeatureNode* transcriptTree,
                           ostream& outFh) const;
    void processGene(GxfParser *gxfParser,
                     const GxfFeature* geneFeature,
                     ostream& outFh) const;
    public:
    /* Constructor */
    GeneMapper(const FeatureTransMap* featureTransMap):
        fFeatureTransMap(featureTransMap) {
    }

    /* Map a GFF3/GTF */
    void mapGxf(GxfParser *gxfParser,
                ostream& outFh) const;
};

#endif
