# rapier

Zorro + Darwinex MT4 setup on Linux via Docker

![](img/zorro.jpg)

Based on [nevmerzhitsky/headless-metatrader4](https://github.com/nevmerzhitsky/headless-metatrader4)

This repository contains binaries which are freely available from:

- [Zorro Download Page](https://zorro-project.com/download.php)
- [Download the Darwinex MT4/MT5 Platform](https://help.darwinex.com/download-the-darwinex-mt4-mt5-platforms)

If I am inadvertently in violation of their license by including them
here then I will happily remove them.

# Clone or fork this repo (and make it private)

Make a _private_ copy for yourself so you can update the files in
`secrets/` without giving away your credentials.

There's probably a better way to do this but this way is quite
convenient.

You can also copy your secrets over the top of the templates provided
immediately prior to building and running the container.

# Install Docker

Choose an AMI, anything that can run Docker is fine.

## Ubuntu 20.04

Install the AWS tools.

```
sudo apt install -y awscli
```

Install docker.

```
sudo apt update
sudo apt install -y docker.io
```

Allow current user to run docker.

```
sudo usermod -aG docker $USER
```

## Amazon Linux 2

Install git.

```
sudo yum install -y git
```

Install docker.

```
sudo amazon-linux-extras install docker
sudo service docker start
```

Make docker auto-start.

```
sudo chkconfig docker on
```

Allow current user to run docker.

```
sudo usermod -a -G docker $USER
```

## (optional) Store docker files in another location

If you have a fast local disk you may want to use it, this won't be
persisted if your spot instance gets terminated though.

Create a file `/etc/docker/daemon.json` to move the docker
storage location. This requires at least 5 GiB currently.

```
{
  "data-root": "/mnt/docker"
}
```

_note: I ended up not using this, but thought I would document it as
I thought it may be useful to someone._

# Build and Run Zorro Container

Clone this repo.

```
git clone git@github.com:<user>/rapier
```

Copy your `secrets/` into place unless they're already committed to the
cloned repo.

(optional) Enable vi mode in the shell so you don't go insane.

```
cp rapier/bashrc .bashrc
cp rapier/inputrc .inputrc
```

Log out and log back in for the `.bashrc` changes + docker permissions
to take effect.

(optional) Update DNS info.

You will need to modify the shell script to have the desired hosted zone
and recordset first.

```
rapier/bin/update-route53.sh
```

(optional) Start a `screen` session so that you can exit the shell and everything will keep running.

```
screen -R
```

Start the container.

```
rapier/bin/run.sh
```

(optional) Detach from the screen session and logout.

```
<Ctrl-A> <Ctrl-D>
exit
```

You can reattach to the session by using `screen -R` again.
