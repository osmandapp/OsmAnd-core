/* -----------------------------------------------------------------------------
 * Based on std_vector.i
 * ----------------------------------------------------------------------------- */

%include <std_vector.i>

%{
#include <QList>
%}

template<class T> class QList {
  public:
	typedef size_t size_type;
	typedef T value_type;
	typedef const value_type& const_reference;
	QList();
	size_type size() const;
	void reserve(size_type n);
	%rename(isEmpty) empty;
	bool empty() const;
	void clear();
	%rename(add) push_back;
	void push_back(const value_type& x);
	%extend {
		const_reference get(int i) throw (std::out_of_range) {
			int size = int(self->size());
			if (i>=0 && i<size)
				return (*self)[i];
			else
				throw std::out_of_range("QList index out of range");
		}
		void set(int i, const value_type& val) throw (std::out_of_range) {
			int size = int(self->size());
			if (i>=0 && i<size)
				(*self)[i] = val;
			else
				throw std::out_of_range("QList index out of range");
		}
	}
};

// bool specialization
template<> class QList<bool> {
  public:
	typedef size_t size_type;
	typedef bool value_type;
	typedef bool const_reference;
	QList();
	size_type size() const;
	void reserve(size_type n);
	%rename(isEmpty) empty;
	bool empty() const;
	void clear();
	%rename(add) push_back;
	void push_back(const value_type& x);
	%extend {
		bool get(int i) throw (std::out_of_range) {
			int size = int(self->size());
			if (i>=0 && i<size)
				return (*self)[i];
			else
				throw std::out_of_range("QList index out of range");
		}
		void set(int i, const value_type& val) throw (std::out_of_range) {
			int size = int(self->size());
			if (i>=0 && i<size)
				(*self)[i] = val;
			else
				throw std::out_of_range("QList index out of range");
		}
	}
};
