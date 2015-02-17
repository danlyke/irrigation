class Response
{
private:
public:
	virtual ~Response();
	virtual bool HasData() = 0;
	virtual bool Write(int fd) = 0;
	virtual bool Done();
	
};
class ResponseError : public Response
{
private :
	char *data;
	int dataPos;
	int dataLength;
public :
	ResponseError(int errorCode);
	~ResponseError();
	bool HasData();
	bool Write(int fd);
};
class ResponseStatic : public Response
{
private :
	char *data;
	int dataPos;
	int dataLength;
public :
	ResponseStatic(const char *contentType, const char *content);
	~ResponseStatic();
	bool HasData();
	bool Write(int fd);
};



class ResponseMovement : public Response
{
private :
	char *data;
	int dataPos;
	int dataLength;
	bool done;
public :
	ResponseMovement(const char *contentType, const char *content);
	~ResponseMovement();
	bool HasData();
	bool Write(int fd);
	bool Done();
	
	void AddContent(const char *content);
	void WriteFinishedBlock();
};
