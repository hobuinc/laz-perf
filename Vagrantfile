# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "precise64"

  config.vm.hostname = "laz-perf"
  config.vm.box_url = "http://files.vagrantup.com/precise64.box"

  config.vm.network :forwarded_port, guest: 5000, host: 5000

  config.vm.provider :virtualbox do |vb|
	  vb.customize ["modifyvm", :id, "--memory", "4096"]
	  vb.customize ["modifyvm", :id, "--cpus", "1"]   
	  vb.customize ["modifyvm", :id, "--ioapic", "on"]
  end  

  #
  ppaRepos = [
	  "ppa:ubuntu-toolchain-r/test",
	  "ppa:boost-latest/ppa"
  ]

  packageList = [
	  "git",
	  "build-essential",
	  "gcc-4.8",
	  "g++-4.8",
	  "libboost1.55-all-dev",
	  "cmake",
	  "lcov"
  ];

  if Dir.glob("#{File.dirname(__FILE__)}/.vagrant/machines/default/*/id").empty?
	  pkg_cmd = ""

	  pkg_cmd << "apt-get update -qq; apt-get install -q -y python-software-properties; "

	  if ppaRepos.length > 0
		  ppaRepos.each { |repo| pkg_cmd << "add-apt-repository -y " << repo << " ; " }
		  pkg_cmd << "apt-get update -qq; "
	  end

	  # install packages we need
	  pkg_cmd << "apt-get install -q -y " + packageList.join(" ") << " ; "
	  config.vm.provision :shell, :inline => pkg_cmd
  end
end
