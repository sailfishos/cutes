Name:    cutes-qt5
Summary: QtScript environment and "interpreter"
Version: 0.0.0
Release: 1

License: LGPLv2.1+
Group:	 System/Shells
URL:     https://git.merproject.org/mer-core/cutes
Source0: %{name}-%{version}.tar.bz2

BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Widgets)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Script)
BuildRequires: cmake >= 2.8
BuildRequires: python >= 2.7
BuildRequires: pkgconfig(tut) >= 0.0.1
BuildRequires: pkgconfig(cor) >= 0.1.6
Provides: cutes = %{version}
Obsoletes: cutes < 0.7.10

%description
QtScript environment and "interpreter"

%package devel
Summary: Development support for cutes extensions
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
%{summary}.

%package tests
Summary:    Tests for cutes
Group:      System Environment/Libraries
Requires:   %{name} = %{version}-%{release}
%description tests
%{summary}.

%package doc
Summary:   Documentation for %{name}
Group:     Documentation
Requires:  %{name} = %{version}-%{release}
%description doc
Man page for %{name}.

%define qt_importdir %{_libdir}/qt5/qml
%define jslibdir %{_datadir}/cutes

%prep
%setup -q -n %{name}-%{version}

%build
%cmake -DUSEQT=5 -DVERSION=%{version}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%files
%defattr(-,root,root,-)
%license LICENSE
%{_bindir}/cutes
%{_libdir}/libcutes-qt5.so.*
%{_libdir}/qt5/cutes/qt/libcutes-core.so
%{qt_importdir}/Mer/Cutes/libcutesqml.so
%{qt_importdir}/Mer/Cutes/qmldir
%{jslibdir}/*.js

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcutes-qt5.so
%dir %{_includedir}/cutes
%{_includedir}/cutes/*.hpp
%{_libdir}/cutes/bin/preprocess_bridge.py

%files tests
%defattr(-,root,root,-)
/opt/tests/cutes/*

%files doc
%defattr(-,root,root,-)
%{_mandir}/man1/cutes.*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig
