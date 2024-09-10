FROM dockcross/windows-static-x86

RUN wget https://github.com/conan-io/conan/releases/download/2.6.0/conan-2.6.0-amd64.deb
RUN dpkg -i conan-2.6.0-amd64.deb