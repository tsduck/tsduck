//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for utilities in tsAlgorithm.h
//
//----------------------------------------------------------------------------

#include "tsAlgorithm.h"
#include "tsunit.h"
#include <cstdarg>

// We test our algorithms on elements of type 'Id'
namespace {
    using Id = int;
    using IdSet = std::set<Id>;
    using SetOfIdSet = std::set<IdSet>;
}


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class AlgorithmTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testEnumerateCombinations();

    TSUNIT_TEST_BEGIN(AlgorithmTest);
    TSUNIT_TEST(testEnumerateCombinations);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(AlgorithmTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void AlgorithmTest::beforeTest()
{
}

// Test suite cleanup method.
void AlgorithmTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Display a set of Id's on a stream
namespace {
    std::ostream& operator<< (std::ostream& strm, const IdSet& s)
    {
        strm << "{";
        bool first = true;
        for (auto it : s) {
            strm << (first ? "" : ", ") << it;
            first = false;
        }
        return strm << "}";
    }
}

// Build an IdSet from a variable-list of non-zero values.
// End with a zero value.
namespace {
    IdSet Set (Id id, ...)
    {
        IdSet set;
        va_list ap;
        va_start (ap, id);
        Id i = id;
        while (i != 0) {
            set.insert (i);
            i = va_arg (ap, Id);
        }
        va_end (ap);
        return set;
    }
}

// A functor which removes an IdSet from a SetOfIdSet.
// Return false when passed combination is equal to the marker.
namespace {
    class RemoveIdSet
    {
    private:
        SetOfIdSet& _collection;
        const IdSet& _last;
        bool _found;
    public:
        RemoveIdSet (SetOfIdSet& collection, const IdSet& last):
            _collection (collection),
            _last (last),
            _found (false)
        {
        }
        // Used as predicate for EnumerateCombinations.
        bool operator() (const IdSet& s)
        {
            tsunit::Test::debug() << "AlgorithmTest: combination: " << s << std::endl;
            TSUNIT_EQUAL(1, _collection.erase(s));
            TSUNIT_ASSERT(!_found);
            _found = s == _last;
            return !_found;
        }
    };
}

// Test cases
void AlgorithmTest::testEnumerateCombinations()
{
    IdSet values;            // set of all values
    IdSet fixed;             // set of fixed values in all expected combinations.
    IdSet end;               // combination which ends the search
    SetOfIdSet collection;   // collection of all expected combinations.

    // Enumerate all (5,3) combinations

    values = Set (1, 2, 3, 4, 5, 0);
    fixed.clear();
    end.clear();
    collection.clear();
    collection.insert (Set (1, 2, 3, 0));
    collection.insert (Set (1, 2, 4, 0));
    collection.insert (Set (1, 2, 5, 0));
    collection.insert (Set (1, 3, 4, 0));
    collection.insert (Set (1, 3, 5, 0));
    collection.insert (Set (1, 4, 5, 0));
    collection.insert (Set (2, 3, 4, 0));
    collection.insert (Set (2, 3, 5, 0));
    collection.insert (Set (2, 4, 5, 0));
    collection.insert (Set (3, 4, 5, 0));

    debug() << "AlgorithmTest: 3-elements combinations in " << values
                 << " containing " << fixed
                 << " ending search at " << end << std::endl;

    bool completed = ts::EnumerateCombinations (values, fixed, 3, RemoveIdSet (collection, end));

    debug() << "AlgorithmTest: completed: " << completed << ", remaining combinations: " << collection.size() << std::endl;
    TSUNIT_ASSERT(completed);
    TSUNIT_ASSERT(collection.size() == 0);

    // Enumerate all (5,3) combinations containing {2, 4}

    values = Set (1, 2, 3, 4, 5, 0);
    fixed = Set (2, 4, 0);
    end.clear();
    collection.clear();
    collection.insert (Set (1, 2, 4, 0));
    collection.insert (Set (2, 3, 4, 0));
    collection.insert (Set (2, 4, 5, 0));

    debug() << "AlgorithmTest: 3-elements combinations in " << values
                 << " containing " << fixed
                 << " ending search at " << end << std::endl;

    completed = ts::EnumerateCombinations (values, fixed, 3, RemoveIdSet (collection, end));

    debug() << "AlgorithmTest: completed: " << completed << ", remaining combinations: " << collection.size() << std::endl;
    TSUNIT_ASSERT(completed);
    TSUNIT_ASSERT(collection.size() == 0);

    // Enumerate all (5,3) combinations, stopping at {2, 3, 5}

    values = Set (1, 2, 3, 4, 5, 0);
    fixed.clear();
    end = Set (2, 3, 5, 0);
    collection.clear();
    collection.insert (Set (1, 2, 3, 0));
    collection.insert (Set (1, 2, 4, 0));
    collection.insert (Set (1, 2, 5, 0));
    collection.insert (Set (1, 3, 4, 0));
    collection.insert (Set (1, 3, 5, 0));
    collection.insert (Set (1, 4, 5, 0));
    collection.insert (Set (2, 3, 4, 0));
    collection.insert (Set (2, 3, 5, 0));
    collection.insert (Set (2, 4, 5, 0));
    collection.insert (Set (3, 4, 5, 0));

    debug() << "AlgorithmTest: 3-elements combinations in " << values
                 << " containing " << fixed
                 << " ending search at " << end << std::endl;

    completed = ts::EnumerateCombinations (values, fixed, 3, RemoveIdSet (collection, end));

    debug() << "AlgorithmTest: completed: " << completed << ", remaining combinations: " << collection.size() << std::endl;
    TSUNIT_ASSERT(!completed);
}
