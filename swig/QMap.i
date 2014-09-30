/* -----------------------------------------------------------------------------
 * Based on std_map.i
 * ----------------------------------------------------------------------------- */

%include <std_map.i>

%{
#include <QMap>
%}

// exported class

template<class K, class T> class QMap {
	// add typemaps here
  public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef K key_type;
	typedef T mapped_type;
	QMap();
	QMap(const QMap<K,T> &);
	
	unsigned int size() const;
	bool empty() const;
	void clear();
	%extend {
		const T& get(const K& key) throw (std::out_of_range) {
			QMap<K,T >::iterator i = self->find(key);
			if (i != self->end())
				return i->second;
			else
				throw std::out_of_range("key not found");
		}
		void set(const K& key, const T& x) {
			(*self)[key] = x;
		}
		void del(const K& key) throw (std::out_of_range) {
			QMap<K,T >::iterator i = self->find(key);
			if (i != self->end())
				self->erase(i);
			else
				throw std::out_of_range("key not found");
		}
		bool has_key(const K& key) {
			QMap<K,T >::iterator i = self->find(key);
			return i != self->end();
		}
	}
};
