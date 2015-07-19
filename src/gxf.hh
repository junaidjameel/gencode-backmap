/*
 * Extremely naive and specialized GFF3 and GTF parsers.
 *
 * The goal preserve the exact structure of the GFF3/GTF files, including
 * comments, and only update coordinates (and occasionally split lines).
 * Neither parser in the kent tree design for this.
 *
 * These parser assume the ordering of the GENCODE GFF3/GTF files.
 */

#ifndef gxf_hh
#define gxf_hh
#include "typeOps.hh"
#include <queue>
#include <stdexcept>
using namespace std;

/* it seems so stupid to need to keep writing one-off GFF/GTF parsers */

class GxfFeature;
class FIOStream;

typedef enum {
    GFF3_FORMAT,
    GTF_FORMAT,
} GxfFormat;

/* vector of feature objects */
typedef vector<const GxfFeature*>  GxfFeatureVector;

/*
 * GxF base record type.  Use instanceOf to determine actually type
 */
class GxfRecord {
    public:
    /* destructor */
    virtual ~GxfRecord() {
    }

    /* return record as a string */
    virtual string toString() const = 0;
};

/*
 * non-feature line.
 */
class GxfLine: public string, public GxfRecord {
    public:
    GxfLine(const string& line):
        string(line) {
    }

    /* destructor */
    virtual ~GxfLine() {
    }

    /* return record as a string */
    virtual string toString() const {
        return *this;
    }
};

/* attribute/value pair */
class AttrVal {
    public:
    const string fName;
    const string fVal;
    const bool fQuoted;

    AttrVal(const string& name, const string& val, bool quoted):
        fName(name), fVal(val), fQuoted(quoted) {
        if (stringEmpty(fName)) {
            throw invalid_argument("empty attribute name");
        }
        if (stringEmpty(fVal)) {
            throw invalid_argument("empty attribute value");
        }
    }
};
/* list of attributes */
typedef vector<AttrVal> AttrVals;

/*
 * A row parsed from a GTF/GFF file. Immutable object.
 */
class GxfFeature: public GxfRecord {
public:
    // Standard feature names
    static const string GENE;
    static const string TRANSCRIPT;
    static const string EXON;
    static const string CDS;
    static const string START_CODON;
    static const string UTR;
    static const string STOP_CODON;
    static const string STOP_CODON_REDEFINED_AS_SELENOCYSTEINE;

    
    // columns parsed from file.
    const string fSeqid;
    const string fSource;
    const string fType;
    const int fStart;
    const int fEnd;
    const string fScore;
    const string fStrand;
    const string fPhase;
    const AttrVals fAttrs;

    protected:
    string baseColumnsAsString() const;
    
    public:
    /* construct a new feature object */
    GxfFeature(const string& seqid, const string& source, const string& type,
               int start, int end, const string& score, const string& strand,
               const string& phase, const AttrVals& attrs):
        fSeqid(seqid), fSource(source), fType(type),
        fStart(start), fEnd(end),
        fScore(score), fStrand(strand),
        fPhase(phase), fAttrs(attrs) {
    }

    /* destructor */
    virtual ~GxfFeature() {
    }

    /* get a attribute, NULL if it doesn't exist */
    const AttrVal* findAttr(const string& name) const;

    /* get a attribute, error it doesn't exist */
    const AttrVal* getAttr(const string& name) const;
};

/**
 * gff3 or gtf parser.
 */
class GxfParser {
    private:
    FIOStream* fIn;  // input stream
    GxfFormat fGxfFormat; // format of file
    queue<const GxfRecord*> fPending; // FIFO of pushed records to be read before file

    StringVector splitFeatureLine(const string& line) const;
    const GxfFeature* createGff3Feature(const StringVector& columns) const;
    const GxfFeature* createGtfFeature(const StringVector& columns) const;
    const GxfFeature* createGxfFeature(const StringVector& columns) const;
    const GxfRecord* read();
    
    public:
    /* constructor that opens file, which maybe compressed */
    GxfParser(const string& fileName,
              GxfFormat gxfFormat);

    /* destructor */
    ~GxfParser();

    /* Read the next record, either queued by push() or from the file , use
     * instanceOf to determine the type.  Return NULL on EOF.
     */
    const GxfRecord* next();

    /* Return a record to be read before the file. */
    void push(const GxfRecord* gxfRecord) {
        fPending.push(gxfRecord);
    }
};

/**
 * Tree container for a GxfFeature object and children
 */
class GxfFeatureNode {
    public:
    const GxfFeature* fFeature;
    vector<const GxfFeatureNode*> fChildren;

    GxfFeatureNode(const GxfFeature* feature):
        fFeature(feature) {
    }

    ~GxfFeatureNode() {
        delete fFeature;
        for (size_t i = 0; i < fChildren.size(); i++) {
            delete fChildren[i];
        }
    }

};

#endif
