#!/bin/bash

mkdir -p /libraryit/ebooks
mkdir -p /libraryit/papers
mkdir -p /libraryit/sourcecode
mkdir -p /libraryit/docs

groupadd readonly
groupadd staff

useradd -M member
useradd -M contributor
useradd -M librarian

echo "member:member123" | chpasswd
echo "contributor:contrib456" | chpasswd
echo "librarian:lib789" | chpasswd

usermod -aG readonly member

usermod -aG staff contributor
usermod -aG staff librarian
(echo member123; echo member123) | smbpasswd -a member
(echo contrib456; echo contrib456) | smbpasswd -a contributor
(echo lib789; echo lib789) | smbpasswd -a librarian
chmod -R 770 /libraryit/ebooks
chmod -R 770 /libraryit/papers
chmod -R 750 /libraryit/sourcecode
chmod -R 777 /libraryit/docs

chown -R root:staff /libraryit/ebooks
chown -R root:staff /libraryit/papers
chown -R root:staff /libraryit/sourcecode
chown -R root:staff /libraryit/docs
exec smbd --foreground --no-process-group
