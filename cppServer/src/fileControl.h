#pragma once
#include <string>
#include <fstream>

class file_interface
{
public:
	virtual ~file_interface() = default;;
	virtual bool selectFile(const std::string filename) = 0;
	virtual void addToFile(const std::string data) = 0;
	virtual void readFromFile(std::string& data) = 0;
	virtual void clearContentFile() = 0;
};

class production_file : public file_interface
{
public:
	bool selectFile(const std::string filename) override
	{
		m_filename = filename;

		if (!isFileExist(m_filename))
		{
			std::print("[*] File does not exist, creating it: {}\n", m_filename);
			std::ofstream ofs(m_filename);
			if (!ofs)
			{
				std::print("[!] Error creating file: {}\n", m_filename);
				return false;
			}
			ofs.close();
		}

		//m_streamFile.open(m_filename, std::ios::in | std::ios::out | std::ios::trunc); // if already exists, truncate it (clear content)
		m_streamFile.open(m_filename, std::ios::in | std::ios::out | std::ios::ate); // if already exists, put cursor At The End (ate)
		if (!m_streamFile.is_open())
		{
			std::print("[!] Error opening file: {}\n", m_filename);
			return false;
		}
		std::print("[*] File selected: {}\n", m_filename);
		m_isFileSelected = true;
		return m_isFileSelected;
	};
	void addToFile(const std::string data) override
	{
		if (m_isFileSelected)
		{
			// Write
			m_streamFile << data;
			std::print("[*] Data added to file: {}\n", data);

		}
	};

	bool isFileExist(const std::string& filename)
	{
		std::fstream s;
		s.open(filename, std::ios::in);

		if (!s.is_open()) // Check if file exists
		{
			std::print("[!] Error opening file: {}\n", filename);
			return false;
		}
		m_streamFile.close();
		return true;
	}

	void readFromFile(std::string& data) override
	{
		if (!m_isFileSelected)
		{
			return;
		}

		m_streamFile.clear(); // Clear flags
		m_streamFile.seekg(0, std::ios::beg); // Cursor at the beginning
		data.assign((std::istreambuf_iterator<char>(m_streamFile)), std::istreambuf_iterator<char>()); // Read all content
	};
	void clearContentFile() override
	{
		if (!m_isFileSelected)
		{
			return;
		}
		if (!m_streamFile.is_open())
		{
			return;
		}
		m_streamFile.close();
		m_streamFile.open(m_filename, std::ios::in | std::ios::out | std::ios::trunc);
	};

	~production_file() override { m_streamFile.close(); };
private:
	std::fstream m_streamFile;
	bool m_isFileSelected = false;
	std::string m_filename;
};