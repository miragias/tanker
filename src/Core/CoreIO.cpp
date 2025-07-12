static std::vector<char> readFile(const std::string &filename) 
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

std::vector<char*> LoadFileList(const char* filePath)
{
    std::vector<char*> fileList;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file list: " + std::string(filePath));
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            // strdup allocates memory, you must free() each char* later
            fileList.push_back(_strdup(line.c_str()));
        }
    }

    return fileList;
}
