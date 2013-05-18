#ifndef FILTER_H
#define FILTER_H

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <sstream>
#include <iterator>

#include "mat.h"
#include "sgnode.h"
#include "common.h"
#include "soar_interface.h"

/*
 Wrapper for all filter value types so we can cache them uniformly.
*/
class filter_val {
public:
	virtual ~filter_val() {}
	virtual std::string get_string() const = 0;
	virtual filter_val *clone() const = 0;
	virtual filter_val &operator=(const filter_val &rhs) = 0;
	virtual bool operator==(const filter_val &rhs) const = 0;
};

template <typename T>
class filter_val_c : public filter_val {  // c for concrete
public:
	filter_val_c(const T &v) : v(v) {}
	virtual ~filter_val_c() {}

	std::string get_string() const {
		std::stringstream ss;
		ss << v;
		return ss.str();
	}
	
	filter_val *clone() const {
		return new filter_val_c<T>(v);
	}
	
	filter_val &operator=(const filter_val &rhs) {
		const filter_val_c<T> *c = dynamic_cast<const filter_val_c<T>*>(&rhs);
		assert(c);
		v = c->v;
		return *this;
	}
	
	bool operator==(const filter_val &rhs) const {
		const filter_val_c<T> *c = dynamic_cast<const filter_val_c<T>*>(&rhs);
		if (!c) {
			return false;
		}
		return v == c->v;
	}
	
	T get_value() const {
		return v;
	}
	
	void set_value(const T &n) {
		v = n;
	}
	
private:
	T v;
};

template <>
class filter_val_c <const sgnode*> : public filter_val {
public:
	filter_val_c(const sgnode *v) : v(v) {}
	virtual ~filter_val_c() {}
	
	std::string get_string() const {
		return v->get_name();
	}
	
	filter_val *clone() const {
		return new filter_val_c<const sgnode *>(v);
	}
	
	filter_val &operator=(const filter_val &rhs) {
		const filter_val_c<const sgnode *> *c = dynamic_cast<const filter_val_c<const sgnode *>*>(&rhs);
		assert(c);
		v = c->v;
		return *this;
	}
	
	bool operator==(const filter_val &rhs) const {
		const filter_val_c<const sgnode *> *c = dynamic_cast<const filter_val_c<const sgnode *>*>(&rhs);
		if (!c) {
			return false;
		}
		return v == c->v;
	}
	
	const sgnode *get_value() const {
		return v;
	}
	
	void set_value(const sgnode *n) {
		v = n;
	}
	
private:
	const sgnode *v;
};

/*
 Convenience functions for getting filter outputs as specific values
 with error checking
 */
template <class T>
inline bool get_filter_val (const filter_val *fv, T &v) {
	const filter_val_c<T> *cast;
	
	if (!(cast = dynamic_cast<const filter_val_c<T>*>(fv))) {
		return false;
	}
	v = cast->get_value();
	return true;
}

/*
 Specialization for floats to allow getting floats, doubles, and ints
*/
template <>
inline bool get_filter_val<double>(const filter_val *fv, double &v) {
	const filter_val_c<double> *dfv;
	const filter_val_c<float> *ffv;
	const filter_val_c<int> *ifv;
	
	if ((dfv = dynamic_cast<const filter_val_c<double>*>(fv))) {
		v = dfv->get_value();
		return true;
	}
	if ((ffv = dynamic_cast<const filter_val_c<float>*>(fv))) {
		v = ffv->get_value();
		return true;
	}
	
	if ((ifv = dynamic_cast<const filter_val_c<int>*>(fv))) {
		v = ifv->get_value();
		return true;
	}
	
	return false;
}

template <class T>
inline bool set_filter_val (filter_val *fv, const T &v) {
	filter_val_c<T> *cast;
	
	if (!(cast = dynamic_cast<filter_val_c<T>*>(fv))) {
		return false;
	}
	cast->set_value(v);
	return true;
}

template<class T>
class ctlist_listener {
public:
	virtual void handle_ctlist_add(const T *e) {}
	virtual void handle_ctlist_remove(const T *e) {}
	virtual void handle_ctlist_change(const T *e) {}
};

/*
 A list that keeps track of changes made to it, so that users can respond
 to only the things that changed. Both the filter output list and filter
 input lists derive from this class. This class also assumes that it owns
 the memory of any element added to it, so it will free that memory when
 items are removed from the list.
*/
template<class T>
class change_tracking_list {
public:
	change_tracking_list() : m_added_begin(0) {}
	
	~change_tracking_list() {
		for (int i = 0; i < current.size(); ++i) {
			delete current[i];
		}
		clear_removed();
	}
	
	void add(T* v) {
		current.push_back(v);
		for (int i = 0; i < listeners.size(); ++i) {
			listeners[i]->handle_ctlist_add(v);
		}
	}
	
	void remove(const T* v) {
		bool found = false;
		for (int i = 0; i < current.size(); ++i) {
			if (current[i] == v) {
				removed.push_back(current[i]);
				current.erase(current.begin() + i);
				if (i < m_added_begin) {
					--m_added_begin;
				}
				found = true;
				break;
			}
		}
		assert(found);
		for (int i = 0; i < changed.size(); ++i) {
			if (changed[i] == v) {
				changed.erase(changed.begin() + i);
				break;
			}
		}
		for (int i = 0; i < listeners.size(); ++i) {
			listeners[i]->handle_ctlist_remove(v);
		}
	}
	
	void change(const T *v) {
		for(int i = 0; i < current.size(); ++i) {
			if (current[i] == v) {
				if (i < m_added_begin &&
				    find(changed.begin(), changed.end(), current[i]) == changed.end())
				{
					changed.push_back(current[i]);
					for (int i = 0; i < listeners.size(); ++i) {
						listeners[i]->handle_ctlist_change(current[i]);
					}
				}
				return;
			}
		}
		assert(false);
	}
	
	void clear_changes() {
		m_added_begin = current.size();
		changed.clear();
		clear_removed();
	}
	
	/*
	 This is kind of like the opposite of clear_changes, in that it
	 makes everything a new addition.
	*/
	void reset() {
		changed.clear();
		clear_removed();
		m_added_begin = 0;
	}
	
	void clear() {
		current.clear();
		changed.clear();
		clear_removed();
		m_added_begin = 0;
	}
		
	int num_current() const {
		return current.size();
	}
	
	int num_changed() const {
		return changed.size();
	}
	
	int num_removed() const {
		return removed.size();
	}
	
	T* get_current(int i) {
		return current[i];
	}
	
	const T* get_current(int i) const {
		return current[i];
	}
	
	T* get_changed(int i) {
		return changed[i];
	}
	
	const T* get_changed(int i) const {
		return changed[i];
	}
	
	T* get_removed(int i) {
		return removed[i];
	}
	
	const T* get_removed(int i) const {
		return removed[i];
	}

	int first_added() const {
		return m_added_begin;
	}
	
	void listen(ctlist_listener<T> *l) {
		listeners.push_back(l);
	}
	
	void unlisten(ctlist_listener<T> *l) {
		typename std::vector<ctlist_listener<T>*>::iterator i;
		i = std::find(listeners.begin(), listeners.end(), l);
		if (i != listeners.end()) {
			listeners.erase(i);
		}
	}
	
private:
	void clear_removed() {
		for (int i = 0; i < removed.size(); ++i) {
			delete removed[i];
		}
		removed.clear();
	}
	
	std::vector<T*> current;
	std::vector<T*> removed;
	std::vector<T*> changed;
	
	// Index of the first new element in the current list
	int m_added_begin;
	
	std::vector<ctlist_listener<T>*> listeners;
};

class filter;

/*
 Every filter generates a list of filter values as output, even if
 the list is empty or a singleton.
*/
typedef change_tracking_list<filter_val> filter_output;

/*
 A filter parameter set represents one complete input into a filter. It's
 just a list of pairs <parameter name, value>.
*/
typedef std::vector<std::pair<std::string, filter_val*> > filter_params;

/*
 Each filter takes a number of input parameters. Each of those parameters
 is in the form of an output list. The derived classes of this abstract
 base class are responsible for combining those separate output lists into
 a single list of parameter sets. For example, the output parameter set
 could be the Cartesian product of all elements in each input list.
 
 I'm assuming that this class owns the memory of the filters that are
 added to it.
*/
class filter_input : public change_tracking_list<filter_params> {
public:
	struct param_info {
		std::string name;
		filter *in_fltr;
	};
	
	typedef std::vector<param_info> input_table;
	
	virtual ~filter_input();
	
	bool update();
	void add_param(std::string name, filter *f);

	virtual void combine(const input_table &inputs) = 0;

private:
	input_table input_info;
};

typedef ctlist_listener<filter_params> filter_input_listener;

class null_filter_input : public filter_input {
public:
	void combine(const input_table &inputs) {}
};

/*
 Input class that just concatenates all separate lists into
 a single parameter set list, with each parameter set being a single
 element.
*/
class concat_filter_input : public filter_input {
public:
	void combine(const input_table &inputs);

private:
	std::map<filter_val*, filter_params*> val2params;
};

/*
 Input class that takes the Cartesian product of all input lists.
*/
class product_filter_input : public filter_input {
public:
	void combine(const input_table &inputs);
	
private:
	void gen_new_combinations(const input_table &inputs);
	void erase_param_set(filter_params *s);
	
	typedef std::list<filter_params*> param_set_list;
	typedef std::map<filter_val*, param_set_list > val2param_map;
	val2param_map val2params;
};

/*
 The filter is the basic query unit in SVS. Each filter takes a list of
 parameter sets generated by the filter_input class and produces a single
 output list. Soar can "mix-and-match" filters by plugging their outputs
 into inputs of other filters. This is done by specifying the desired
 filter plumbing on the SVS command link.
 
 Filter outputs are updated once every output phase. Updating a filter's
 output is recursive: the filter will first request an update on its
 input, which in turn requests updates on all filters feeding into the
 input. Filters should also try to cache outputs when possible to avoid
 unnecessary computation.
*/
class filter {
public:
	filter(Symbol *root, soar_interface *si, filter_input *in) 
	: root(root), si(si), status_wme(NULL), input(in)
	{
		if (input == NULL) {
			input = new null_filter_input();
		}
		if (root && si) {
			si->find_child_wme(root, "status", status_wme);
		}
	}
	
	virtual ~filter() {
		delete input;
	}
	
	void set_status(const std::string &msg) {
		if (status == msg) {
			return;
		}
		status = msg;
		if (status_wme) {
			si->remove_wme(status_wme);
		}
		if (root && si) {
			status_wme = si->make_wme(root, si->get_common_syms().status, status);
		}
	}
	
	void add_output(filter_val *v, const filter_params *p) {
		output.add(v);
		output2params[v] = p;
	}
	
	void get_output_params(filter_val *v, const filter_params *&p) {
		if (!map_get(output2params, v, p)) {
			p = NULL;
		}
	}
	
	void remove_output(filter_val *v) {
		output.remove(v);
		output2params.erase(v);
	}
	
	void change_output(filter_val *v) {
		output.change(v);
	}
	
	filter_output *get_output() {
		return &output;
	}
	
	bool update() {
		if (!input->update()) {
			set_status("Errors in input");
			output.clear();
			input->reset();
			return false;
		}
		
		if (!update_outputs()) {
			output.clear();
			input->reset();
			return false;
		}
		set_status("success");
		input->clear_changes();
		return true;
	}
	
	const filter_input *get_input() const {
		return input;
	}
	
	void listen_for_input(filter_input_listener *l) {
		input->listen(l);
	}
	
	void unlisten_for_input(filter_input_listener *l) {
		input->unlisten(l);
	}

	void mark_stale(const filter_params *s) {
		input->change(s);
	}

private:
	virtual bool update_outputs() = 0;
	
	filter_input *input;
	filter_output output;
	std::string status;
	soar_interface *si;
	Symbol *root;
	wme *status_wme;
	std::map<filter_val*, const filter_params*> output2params;
};

/*
 This type of filter assumes a one-to-one mapping of outputs to input
 parameter sets. It's also assumed that each output is only dependent
 on one parameter set. This is in contrast to filters that perform some
 kind of quantification over its inputs; returning the closest object,
 for example.
*/
class map_filter : public filter {
public:
	map_filter(Symbol *root, soar_interface *si, filter_input *input) : filter(root, si, input) {}
	
	/*
	 All created filter_vals are owned by the output list and cleaned
	 up there, so don't do it here.
	*/
	virtual ~map_filter() {}
	
	/*
	 Compute the output from parameters. If called with a new
	 parameter set, out will be NULL, and the implementation should
	 set it to a new filter_val object (which will be owned by the
	 output list). Otherwise, out will point to a valid filter_val and
	 the implementation should change its value. If the value is
	 actually changed, the changed output argument should be set to
	 true. The implementation should return false if an error occurs.
	 */
	virtual bool compute(const filter_params *params, filter_val *&out, bool &changed) = 0;
	
	/*
	 Some derived classes might allocate memory associated with each
	 output. They should override this function so they know when
	 to deallocate that memory.
	*/
	virtual void output_removed(const filter_val *out) { }
	
	bool update_outputs() {
		const filter_input* input = get_input();
		std::vector<const filter_params*>::iterator j;
		
		for (int i = input->first_added(); i < input->num_current(); ++i) {
			filter_val *v = NULL;
			bool changed = false;
			if (!compute(input->get_current(i), v, changed)) {
				return false;
			}
			add_output(v, input->get_current(i));
			io_map[input->get_current(i)] = v;
		}
		for (int i = 0; i < input->num_removed(); ++i) {
			io_map_t::iterator r = io_map.find(input->get_removed(i));
			assert(r != io_map.end());
			remove_output(r->second);
			output_removed(r->second);
			io_map.erase(r);
		}
		for (int i = 0; i < input->num_changed(); ++i) {
			if (!update_one(input->get_changed(i))) {
				return false;
			}
		}
		for (j = stale.begin(); j != stale.end(); ++j) {
			if (!update_one(*j)) {
				return false;
			}
		}
		stale.clear();
		return true;
	}
	
	void reset() {}

private:
	bool update_one(const filter_params *params) {
		filter_val *v = io_map[params];
		bool changed = false;
		if (!compute(params, v, changed)) {
			return false;
		}
		if (changed) {
			change_output(v);
		}
		return true;
	}
	
	typedef std::map<const filter_params*, filter_val*> io_map_t;
	io_map_t io_map;
	std::vector<const filter_params*> stale;
};

/*
 User-defined filters should derive from this class so that they don't
 have to work with filter_val* directly. Assumes that the filter only
 returns one type of output.
*/
template <class T>
class typed_map_filter : public map_filter {
public:
	typed_map_filter(Symbol *root, soar_interface *si, filter_input *input)
	: map_filter(root, si, input)
	{}
	
	virtual ~typed_map_filter() {}
	
	virtual bool compute(const filter_params *params, bool adding, T &out, bool &changed) = 0;
	virtual void output_removed(const T &out) { }
	
private:
	bool compute(const filter_params *params, filter_val *&out, bool &changed) {
		bool success;
		T val;
		if (out != NULL) {
			success = get_filter_val(out, val);
			assert(success);
		}
		success = compute(params, out == NULL, val, changed);
		if (!success) {
			return false;
		}
		if (!out) {
			out = new filter_val_c<T>(val);
		} else {
			success = set_filter_val(out, val);
			assert(success);
		}
		return true;
	}
	
	void output_removed(const filter_val *out) {
		T val;
		bool success = get_filter_val(out, val);
		assert(success);
		output_removed(val);
	}
};

/*
 This type of filter processes all inputs and produces a single
 output. 
*/
template<typename T>
class reduce_filter : public filter {
public:
	reduce_filter(Symbol *root, soar_interface *si, filter_input *input)
	: filter(root, si, input), output(NULL)
	{}
	
	virtual ~reduce_filter() {}
	
	bool update_outputs() {
		T new_val = value;
		const filter_input *input = get_input();
		for (int i = input->first_added(); i < input->num_current(); ++i) {
			if (!input_added(input->get_current(i), new_val)) {
				return false;
			}
		}
		for (int i = 0; i < input->num_changed(); ++i) {
			if (!input_changed(input->get_changed(i), new_val)) {
				return false;
			}
		}
		for (int i = 0; i < input->num_removed(); ++i) {
			if (!input_removed(input->get_removed(i), new_val)) {
				return false;
			}
		}
		
		if (!output && input->num_current() > 0) {
			output = new filter_val_c<T>(new_val);
			add_output(output, NULL);
		} else if (output && input->num_current() == 0) {
			remove_output(output);
			output = NULL;
		} else if (output && value != new_val) {
			bool success = set_filter_val(output, new_val);
			assert(success);
			change_output(output);
		}
		value = new_val;
		return true;
	}
	
private:
	virtual bool input_added(const filter_params *params, T &val) = 0;
	virtual bool input_changed(const filter_params *params, T &val) = 0;
	virtual bool input_removed(const filter_params *params, T &val) = 0;
	
	filter_val_c<T> *output;
	T value;
};

class rank_filter : public filter {
public:
	rank_filter(Symbol *root, soar_interface *si, filter_input *input)
	: filter(root, si, input), output(NULL), old(NULL)
	{}

	virtual bool rank(const filter_params *params, double &r) = 0;

private:
	bool update_outputs() {
		const filter_input *input = get_input();
		double r;
		const filter_params *p;
		for (int i = input->first_added(); i < input->num_current(); ++i) {
			p = input->get_current(i);
			if (!rank(p, r)) {
				return false;
			}
			elems.push_back(make_pair(r, p));
		}
		for (int i = 0; i < input->num_changed(); ++i) {
			p = input->get_changed(i);
			if (!rank(p, r)) {
				return false;
			}
			bool found = false;
			for (int j = 0; j < elems.size(); ++j) {
				if (elems[j].second == p) {
					elems[j].first = r;
					found = true;
					break;
				}
			}
			assert(found);
		}
		for (int i = 0; i < input->num_removed(); ++i) {
			p = input->get_removed(i);
			bool found = false;
			for (int j = 0; j < elems.size(); ++j) {
				if (elems[j].second == p) {
					elems.erase(elems.begin() + j);
					found = true;
					break;
				}
			}
			assert(found);
		}

		if (!elems.empty()) {
			std::pair<double, const filter_params *> m = *std::max_element(elems.begin(), elems.end());
			if (m.second != old) {
				if (output) {
					remove_output(output);
				}
				output = new filter_val_c<double>(m.first);
				add_output(output, m.second);
				old = m.second;
			} else {
				assert(output);
				set_filter_val(output, m.first);
				change_output(output);
			}
		} else if (output) {
			remove_output(output);
			output = NULL;
		}
		return true;
	}

	std::vector<std::pair<double, const filter_params*> > elems;
	filter_val *output;
	const filter_params *old;
};

/*
 Filters that don't take any inputs and always outputs the same value
*/
template <class T>
class const_filter : public filter {
public:
	const_filter(const T &v) : filter(NULL, NULL, NULL), added(false), v(v) {}
	
	bool update_outputs() {
		if (!added) {
			add_output(new filter_val_c<T>(v), NULL);
			added = true;
		}
		return true;
	}
		
private:
	T v;
	bool added;
};

/*
 Passes an arbitrary element in each input parameter set to the output
 list. This filter is intended to be used with concat_filter_input to
 implement a "combine" filter that multiplexes an arbitrary number of
 inputs into a single list.
*/
class passthru_filter : public map_filter {
public:
	passthru_filter(Symbol *root, soar_interface *si, filter_input *input)
	: map_filter(root, si, input)
	{}
	
	bool compute(const filter_params *params, filter_val *&out, bool &changed) {
		if (params->empty()) {
			return false;
		}
		if (out == NULL) {
			out = params->begin()->second->clone();
			changed = true;
		} else {
			changed = (*out == *params->begin()->second);
			if (changed) {
				*out = *params->begin()->second;
			}
		}
		return true;
	}
};

template <typename T>
inline bool get_filter_param(filter *f, const filter_params *params, const std::string &name, T &val) {
	const filter_val *fv;
	std::stringstream ss;
	filter_params::const_iterator i;
	bool found = false;
	for (i = params->begin(); i != params->end(); ++i) {
		if (i->first == name) {
			fv = i->second;
			found = true;
			break;
		}
	}
	if (!found) {
		return false;
	}
	if (!get_filter_val(fv, val)) {
		if (f) {
			ss << "parameter \"" << name << "\" has wrong type";
			f->set_status(ss.str());
		}
		return false;
	}
	return true;
}

#endif
