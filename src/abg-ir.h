#ifndef __ABL_IR_H__
#define __ABL_IR_H__

#include <cstddef>

// A couple of forward declarations to avoid including too much cruft.
namespace std
{
  class string;
}

// Our real stuff
namespace abigail
{
  class location
  {
    location();
    location (unsigned opaque);

  public:

    location(const location&);
    friend class location_manager;
  };

  class location_manager
  {
  public:
    static location create_new_location(const std::string& file,
					size_t line,
					size_t column);
    static void expand_location(const location&,
				std::string& file,
				size_t& line,
				size_t& column);
  };

  /// \brief A declaration that introduces a scope.
  class scope_decl
  {
  public:
    scope_decl();
  };

  class decl
  {
  public:
    decl(const std::string &name);
    std::string&  get_name() const;
  };

  class basic_type
  {
  public:
    basic_type();
    
  };
}

#endif // __ABL_IR_H__
