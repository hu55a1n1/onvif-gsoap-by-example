// Base class for Snapshot and Stream
class Media {

public:
	// Overloaded in derived classes
	Media();
	// virtual ~Media();

	// Returns formatted curl command for downloading snapshot
    virtual std::string getDownloadUri(const std::string mediaUri) = 0;

    // Returns formatted curl command for uploading snapshot
    virtual std::string getUploadUri(const std::string ftpIP, const std::string ftpUser, const std::string ftpPwd) = 0;

    // Public getters exposing private NSVs
    std::string getTimeCreated(void);

protected:
	// Helper methods for subclasses
    static std::string string_format(const std::string fmt_str, ...);
    static std::string formattedTimeStamp(void);

private:
	std::string _timeCreated;
	
};