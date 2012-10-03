Summary: varnish-vmod-header
Name: varnish-vmod-header
Version: 0.1
Release: 1%{?dist}
License: BSD
Group: System Environment/Daemons
Source0: ./libvmod-header.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: varnish > 3.0
BuildRequires: make, autoconf, automake, libtool, python-docutils

%description
libvmod-header

%prep
%setup -n libvmod-header

%build
./autogen.sh
# this assumes that VARNISHSRC is defined on the rpmbuild command line, like this:
# rpmbuild -bb --define 'VARNISHSRC /home/user/rpmbuild/BUILD/varnish-3.0.3' redhat/*spec
./configure VARNISHSRC=%{VARNISHSRC} VMODDIR=/usr/lib64/varnish/vmods/ --prefix=/usr/
make

%install
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}/usr/share/doc/%{name}/ 
cp README.rst %{buildroot}/usr/share/doc/%{name}/
cp LICENSE %{buildroot}/usr/share/doc/%{name}/

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
# /opt/varnish/lib/varnish/vmods/
/usr/lib64/varnish/vmods/
%doc /usr/share/doc/%{name}/*

%if %{IS_EL5}
# seems to be placed here on el5. use rpmbuild --define "IS_EL5 1"
/usr/man/man?/*gz
%else
%{_mandir}/man?/*gz
%endif


%changelog
* Wed Oct 03 2012 Lasse Karstensen <lasse@varnish-software.com> - 0.1-0.20120918
- Initial version.
