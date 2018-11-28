/***
 * Utility class for processing state information.
 */

#pragma once
#include <set>
#include <map>
#include <QString>

class CartaObject;

namespace Carta {

namespace State {

class UtilState {

public:

     /**
      * Parses a string of the form:  key1:value1,key2:value2,etc for
      * keys contained in the QList and returns a map of key value pairs.
      * @param paramsToParse the string to parse.
      * @param keyList a set containing the expected keys in the string.
      * @return a map containing the (key,value) pairs in the string.  An empty map will
      *     be returned is the keys in the string do not match those in the keyList.
      */
     static std::map < QString, QString > parseParamMap( const QString & paramsToParse,
             const std::set < QString > & keyList );

private:
    UtilState();
    virtual ~UtilState();
};
}
}
