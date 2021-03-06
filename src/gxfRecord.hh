/*
 * Basic GFF3/GTF records
 */
#ifndef gxfRecord_hh
#define gxfRecord_hh
#include "typeOps.hh"
#include <stdexcept>
#include <algorithm>

/*
 * Method used to hack PAR ids to be unique when required by format.
 */
typedef enum {
    PAR_ID_HACK_OLD,  // ENSTR method
    PAR_ID_HACK_NEW   // _PAR_Y method
} ParIdHackMethod;

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
typedef vector<GxfRecord*> GxfRecordVector;

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

/* attribute/value pair.  Maybe multi-valued */
class AttrVal {
    private:
    const string fName;
    StringVector fVals;

    static void checkName(const string& name) {
        if (stringEmpty(name)) {
            throw invalid_argument("empty attribute name");
        }
    }
    static void checkVal(const string& val) {
        if (stringEmpty(val)) {
            throw invalid_argument("empty attribute value");
        }
    }

    public:
    AttrVal(const string& name, const string& val):
        fName(name) {
        checkName(name);
        checkVal(val);
        fVals.push_back(val);
    }

    AttrVal(const string& name, const StringVector& vals):
        fName(name), fVals(vals) {
        checkName(name);
        for (int i = 0; i < vals.size(); i++) {
            checkVal(vals[i]);
        }
    }

    /* add a value */
    void addVal(const string& val) {
        checkVal(val);
        fVals.push_back(val);
    }
    
    /* copy constructor */
    AttrVal(const AttrVal& src):
        fName(src.fName), fVals(src.fVals) {
    }

    const string& getName() const {
        return fName;
    }
    const string& getVal(int iVal=0) const {
        return fVals[iVal];
    }
    const StringVector& getVals() const {
        return fVals;
    }
    int size() const {
        return fVals.size();
    }
};

/* vector of attribute/value pointers */
typedef vector<AttrVal*> AttrValVector;

/* list of attributes,  Multi-valued attributes (tag) are stored as multiple 
 * entries. */
class AttrVals: public AttrValVector {
    // n.b.  this keeps pointers rather than values due to reallocation if vector changes
    public:
    /* empty constructor */
    AttrVals() {
    }

    /* copy constructor */
    AttrVals(const AttrVals& src) {
        for (size_t i = 0; i < src.size(); i++) {
            push_back(new AttrVal(*(src[i])));
        }
    }
    
    /* destructor */
    ~AttrVals() {
        for (size_t i = 0; i < size(); i++) {
            delete (*this)[i];
        }
    }

    /* does the attribute exist */
    bool exists(const string& name) const {
        return findIdx(name) >= 0;
    }
    
    /* find the index of the first attribute with name or -1 if not found */
    int findIdx(const string& name) const {
        for (int i = 0; i < size(); i++) {
            if ((*this)[i]->getName() == name) {
                return i;
            }
        }
        return -1;
    }

    
    /* get a attribute, NULL if it doesn't exist */
    const AttrVal* find(const string& name) const {
        int i = findIdx(name);
        if (i < 0) {
            return NULL;
        } else {
            return (*this)[i];
        }
    }
    
    
    /* get a attribute, error it doesn't exist */
    const AttrVal* get(const string& name) const {
        const AttrVal* attrVal = find(name);
        if (attrVal == NULL) {
            throw invalid_argument("Attribute not found: " + name);
        }
        return attrVal;
    }

    /* add an attribute */
    void add(const AttrVal& attrVal) {
        push_back(new AttrVal(attrVal));
    }

    /* insert an attribute at the front */
    void push(const AttrVal& attrVal) {
        insert(begin(), new AttrVal(attrVal));
    }

    /* add or replace an attribute */
    void update(const AttrVal& attrVal) {
        int idx = findIdx(attrVal.getName());
        if (idx < 0) {
            add(attrVal);
        } else {
            delete (*this)[idx];
            (*this)[idx] = new AttrVal(attrVal);
        }
    }

    /* remove an attribute by name, if it exists */
    void remove(const string& attrName) {
        int idx = findIdx(attrName);
        if (idx >= 0) {
            delete (*this)[idx];
            erase(begin()+idx);
        }
    }
};

/*
 * A row parsed from a GTF/GFF file.
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

    // standard attribute names
    static const string ID_ATTR;
    static const string PARENT_ATTR;
    static const string GENE_ID_ATTR;
    static const string GENE_NAME_ATTR;
    static const string GENE_TYPE_ATTR;
    static const string GENE_STATUS_ATTR;
    static const string GENE_HAVANA_ATTR;
    static const string TRANSCRIPT_ID_ATTR;
    static const string TRANSCRIPT_NAME_ATTR;
    static const string TRANSCRIPT_TYPE_ATTR;
    static const string TRANSCRIPT_STATUS_ATTR;
    static const string TRANSCRIPT_HAVANA_ATTR;
    static const string EXON_ID_ATTR;
    static const string EXON_NUMBER_ATTR;
    static const string TAG_ATTR;
    
    /* source names */
    static const string SOURCE_HAVANA;
    static const string SOURCE_ENSEMBL;

    protected:
    // columns parsed from file.
    const string fSeqid;
    const string fSource;
    const string fType;
    const int fStart;
    const int fEnd;
    const string fScore;
    const string fStrand;
    const string fPhase;
    AttrVals fAttrs;     // attribute maybe modified

    public:
    /* construct a new feature object */
    GxfFeature(const string& seqid, const string& source, const string& type,
               int start, int end, const string& score, const string& strand,
               const string& phase, const AttrVals& attrs):
        fSeqid(seqid), fSource(source), fType(type),
        fStart(start), fEnd(end),
        fScore(score), fStrand(strand),
        fPhase(phase), fAttrs(attrs) {
        assert(strand.size() == 1);
        assert(phase.size() == 1);
    }

    /* clone the feature */
    virtual GxfFeature* clone() const = 0;
    
    /* destructor */
    virtual ~GxfFeature() {
    }

    /* convert all columns, except attributes, to a string */
    string baseColumnsAsString() const;

    /* accessors */
    const string& getSeqid() const {
        return fSeqid;
    }
    const string& getSource() const {
        return fSource;
    }
    const string& getType() const {
        return fType;
    }
    int getStart() const {
        return fStart;
    }
    int getEnd() const {
        return fEnd;
    }
    const string& getScore() const {
        return fScore;
    }
    const string& getStrand() const {
        return fStrand;
    }
    const string& getPhase() const {
        return fPhase;
    }

    /* get all attribute */
    const AttrVals& getAttrs() const {
        return fAttrs;
    }

    /* get all attribute */
    AttrVals& getAttrs() {
        return fAttrs;
    }
    
    /* does the attribute exist */
    bool hasAttr(const string& name) const {
        return fAttrs.exists(name);
    }

    /* get a attribute, NULL if it doesn't exist */
    const AttrVal* findAttr(const string& name) const {
        return fAttrs.find(name);
    }

    /* get a attribute, error it doesn't exist */
    const AttrVal* getAttr(const string& name) const {
        return fAttrs.get(name);
    }

    /* get a attribute value, error it doesn't exist */
    const string& getAttrValue(const string& name,
                               int iVal=0) const {
        return getAttr(name)->getVal(iVal);
    }

    /* get a attribute value, default it doesn't exist */
    const string& getAttrValue(const string& name, 
                               const string& defaultVal,
                               int iVal=0) const {
        const AttrVal* attrVal = findAttr(name);
        if (attrVal == NULL) {
            return defaultVal;
        } else {
            return attrVal->getVal(iVal);
        }
    }

    /* get the id based on feature type, or empty string if it doesn't have an
     * id */
    const string& getTypeId() const;
    
    /* get the id based on feature type, or empty string if it doesn't have an
     * id */
    const string& getHavanaTypeId() const;
    
    /* get the name based on feature type, or empty string if it doesn't have an
     * id */
    const string& getTypeName() const;
    
    /* get the biotype based on feature type, or empty string if it doesn't have an
     * id */
    const string& getTypeBiotype() const;
    
    /* get the size of the feature */
    int size() const {
        return (fEnd - fStart)+1;
    }

    /* does this feature overlap another */
    bool overlaps(const GxfFeature* other) const {
        if ((fSeqid != other->fSeqid) || (fStrand != other->fStrand)) {
            return false;
        } else if ((fStart > other->fEnd) || (fEnd < other->fStart)) {
            return false;
        } else {
            return true;
        }
    }

    /* return feature as a string */
    virtual string toString() const;
};

/* vector of feature objects, doesn't own features */
class GxfFeatureVector: public vector<GxfFeature*> {
    public:
    /* free all features in the vector */
    void free() {
        for (int i = 0; i < size(); i++) {
            delete (*this)[i];
        }
        clear();
    }

};

#endif
