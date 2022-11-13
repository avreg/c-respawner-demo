Name:           c-respawner-demo
Version:        0.1.0
Release:        1%{?dist}
Summary:        C-example of respawner for any process

License:        gpl-3
URL:            https://github.com/avreg/c-respawner-demo
Source0:        %{name}-%{version}.tar.gz


BuildRequires:  cmake
# Requires:       libc

%description
 Respawns a single or executable daemon process.
 .
 Support:
   * respawn limit (max attempt) ,
   * respawn timeout,
   * success exit status list,
   * pidfile.


%prep
make clean
cmake -S . -B "build" -DCMAKE_BUILD_TYPE=ReleaseWithDebInfo -DCMAKE_INSTALL_PREFIX="$RPM_BUILD_ROOT"



%build
%make_build


%install
cmake --install build --prefix "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"/usr/bin/
mv "$RPM_BUILD_ROOT"/bin/%{name} "$RPM_BUILD_ROOT"/usr/bin/
strip -s "$RPM_BUILD_ROOT"/usr/bin/%{name}
rm -fR $RPM_BUILD_ROOT/lib64
rm -fR $RPM_BUILD_ROOT/include
mkdir -p $RPM_BUILD_ROOT/%{_docdir}/%{name}/
cp README.md $RPM_BUILD_ROOT/%{_docdir}/%{name}/

%clean
rm -rf $RPM_BUILD_ROOT
make clean

%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}


%changelog
* Sun Nov 13 2022 Andrey Nikitin
- Initially
