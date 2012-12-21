#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <exception>
#include <map>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace std;
using namespace boost;
using namespace boost::interprocess;

// A data structure to manage a memory mapped file.
class MemoryMappedFile : public noncopyable
{
  public:

    MemoryMappedFile(const string filename, const size_t length, 
      const bool keepBacking=false) : _filename(filename), 
      _keepBacking(keepBacking)
    {
      FILE *fp = fopen(filename.c_str(), "wb");
      if (-1 == ftruncate( fileno(fp), length) ) 
      {
        fclose(fp);
        throw(
          runtime_error(
            string("ftruncate returned error: " + 
            string(strerror(errno))) + "\n"
          )
        );
      }
      fclose(fp);

      file_mapping fm(_filename.c_str(), read_write);
      _mr = mapped_region(fm, read_write);
      _pid = getpid();
    }
    
    void destroy()
    {
      file_mapping::remove(_filename.c_str());
      // If the MMF was created by the current process (wasn't forked) and
      // its backing shouldn't be kept, then unlink it.
      if (!_keepBacking & getpid() == _pid)
        unlink(_filename.c_str());
    }

    ~MemoryMappedFile() {}

  public:
    
    void* address() const 
    {
      return _mr.get_address();
    }

    string filename() const
    {
      return _filename;
    }

    void keep_backing(const bool keepBacking) 
    {
      _keepBacking = keepBacking;
    }

    const bool keep_backing() const 
    {
      return _keepBacking;
    }

  protected:
    mapped_region _mr;
    string _filename;
    pid_t _pid;
    bool _keepBacking;
};

// A data structure to hold, add, and remove memory mapped files.
class MemoryMappedFileManager 
{
  public:

    typedef boost::shared_ptr<MemoryMappedFile> MMFPtr;
    typedef map<void*, MMFPtr> MMMDict;

  public:

    // Create a new memory mapped file, add it to the 
    // mamager and return the address of the new resource.
    void* add(const string filename, const size_t length)
    {
      MMFPtr mmf = MMFPtr(new MemoryMappedFile (filename, length));
      // Make sure it's not already there.
      MMMDict::iterator it = _dict.find(mmf->address());
      if (it != _dict.end())
        return NULL;
      _dict.insert(MMMDict::value_type(mmf->address(), mmf));
      return mmf->address();
    }

    const MemoryMappedFile* find(void *ptr)
    {
      MMMDict::iterator it = _dict.find(ptr);
      if (it != _dict.end())
        return it->second.get();
      return NULL;
    }
 
    // Deallocate and remove the resource with the specified address.  
    void remove(void *p)
    {
      MMMDict::iterator it = _dict.find(p);
      if (it == _dict.end())
        return;
  
      // Deallocate the mapped region
      it->second->destroy();
      _dict.erase(it);
    }

  protected:

    MMMDict _dict;
};

