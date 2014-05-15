#!/bin/bash

# Function: cleanExternal(location)
cleanExternal()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"
	
	if ls -1 "$externalPath"/upstream.* >/dev/null 2>&1
	then
		echo "Cleaning '$externalName'..."
		rm -rf "$externalPath"/upstream.*
		if [ $? -ne 0 ]; then
			echo "Failed to remove '$externalName' upstream, aborting..."
			exit $?
		fi
	fi
}
export -f cleanExternal

# Function: configureExternalFromGit(location, url, branch)
configureExternalFromGit()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"
	local gitUrl=$2
	local gitBranch=$3
	
	echo "Processing '$externalName' in '$externalPath'..."
	
	# Check if needs reconfiguring
	if [ -f "$externalPath/stamp" ]; then
		echo "Checking '$externalName'..."
		local lastStamp=""
		if [ -f "$externalPath/.stamp" ]; then
			lastStamp=$(cat "$externalPath/.stamp")
		fi
		local currentStamp=$(cat "$externalPath/stamp")
		echo "Last stamp:    "$lastStamp
		echo "Current stamp: "$currentStamp
		if [ "$lastStamp" != "$currentStamp" ]; then
			echo "Stamps differ, will clean external '$externalName'..."
			cleanExternal "$externalPath"
			cp "$externalPath/stamp" "$externalPath/.stamp"
		fi
	fi
	
	# Check if already configured
	if [ -d "$externalPath/upstream.patched" ]; then
		echo "Skipping '$externalName': already configured"
		exit 0
	fi

	# If there's no original, obtain it
	if [ ! -d "$externalPath/upstream.original" ]; then
		echo "Downloading '$externalName' upstream from $gitUrl [branch $gitBranch] ..."
		git clone $gitUrl "$externalPath/upstream.original" -b $gitBranch --depth=1
		if [ $? -ne 0 ]; then
			echo "Failed to download '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.original"
			exit $?
		fi
	fi
	
	# Clean traces of git
	rm -rf "$externalPath/upstream.original/.git"
}
export -f configureExternalFromGit

# Function: configureExternalFromSvn(location, url)
configureExternalFromSvn()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"
	local svnUrl=$2
	
	echo "Processing '$externalName' in '$externalPath'..."
	
	# Check if needs reconfiguring
	if [ -f "$externalPath/stamp" ]; then
		echo "Checking '$externalName'..."
		local lastStamp=""
		if [ -f "$externalPath/.stamp" ]; then
			lastStamp=$(cat "$externalPath/.stamp")
		fi
		local currentStamp=$(cat "$externalPath/stamp")
		echo "Last stamp:    "$lastStamp
		echo "Current stamp: "$currentStamp
		if [ "$lastStamp" != "$currentStamp" ]; then
			echo "Stamps differ, will clean external '$externalName'..."
			cleanExternal "$externalPath"
			cp "$externalPath/stamp" "$externalPath/.stamp"
		fi
	fi
	
	# Check if already configured
	if [ -d "$externalPath/upstream.patched" ]; then
		echo "Skipping '$externalName': already configured"
		exit 0
	fi

	# If there's no original, obtain it
	if [ ! -d "$externalPath/upstream.original" ]; then
		echo "Downloading '$externalName' upstream from $svnUrl ..."
		svn checkout $svnUrl "$externalPath/upstream.original"
		if [ $? -ne 0 ]; then
			echo "Failed to download '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.original"
			exit $?
		fi
	fi
	
	# Clean traces of svn
	rm -rf "$externalPath/upstream.original/.svn"
}
export -f configureExternalFromSvn

# Function: configureExternalFromTarArchive(location, url)
configureExternalFromTarArchive()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"
	local tarUrl=$2
	
	echo "Processing '$externalName' in '$externalPath'..."
	
	# Check if needs reconfiguring
	if [ -f "$externalPath/stamp" ]; then
		echo "Checking '$externalName'..."
		local lastStamp=""
		if [ -f "$externalPath/.stamp" ]; then
			lastStamp=$(cat "$externalPath/.stamp")
		fi
		local currentStamp=$(cat "$externalPath/stamp")
		echo "Last stamp:    "$lastStamp
		echo "Current stamp: "$currentStamp
		if [ "$lastStamp" != "$currentStamp" ]; then
			echo "Stamps differ, will clean external '$externalName'..."
			cleanExternal "$externalPath"
			cp "$externalPath/stamp" "$externalPath/.stamp"
		fi
	fi

	# Check if already configured
	if [ -d "$externalPath/upstream.patched" ]; then
		echo "Skipping '$externalName': already configured"
		exit 0
	fi

	# If there's no original pack, obtain it
	if [ ! -f "$externalPath/upstream.pack" ]; then
		echo "Downloading '$externalName' upstream from $tarUrl ..."
		curl -L $tarUrl > "$externalPath/upstream.pack"
		if [ $? -ne 0 ]; then
			echo "Failed to download '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.pack"
			exit $?
		fi
	fi

	# Extract upstream if needed
	if [ ! -d "$externalPath/upstream.original" ]; then
		echo "Extracting '$externalName' upstream..."
		mkdir -p "$externalPath/upstream.original"
		tar -xf "$externalPath/upstream.pack" -C "$externalPath/upstream.original" --strip 1
		if [ $? -ne 0 ]; then
			echo "Failed to extract '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.pack" "$externalPath/upstream.original"
			exit $?
		fi
	fi
}
export -f configureExternalFromTarArchive

# Function: configureExternalFromZipArchive(location, url)
configureExternalFromZipArchive()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"
	local tarUrl=$2
	
	echo "Processing '$externalName' in '$externalPath'..."

	# Check if needs reconfiguring
	if [ -f "$externalPath/stamp" ]; then
		echo "Checking '$externalName'..."
		local lastStamp=""
		if [ -f "$externalPath/.stamp" ]; then
			lastStamp=$(cat "$externalPath/.stamp")
		fi
		local currentStamp=$(cat "$externalPath/stamp")
		echo "Last stamp:    "$lastStamp
		echo "Current stamp: "$currentStamp
		if [ "$lastStamp" != "$currentStamp" ]; then
			echo "Stamps differ, will clean external '$externalName'..."
			cleanExternal "$externalPath"
			cp "$externalPath/stamp" "$externalPath/.stamp"
		fi
	fi
	
	# Check if already configured
	if [ -d "$externalPath/upstream.patched" ]; then
		echo "Skipping '$externalName': already configured"
		exit 0
	fi

	# If there's no original pack, obtain it
	if [ ! -f "$externalPath/upstream.pack" ]; then
		echo "Downloading '$externalName' upstream from $tarUrl ..."
		curl -L $tarUrl > "$externalPath/upstream.pack"
		if [ $? -ne 0 ]; then
			echo "Failed to download '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.pack"
			exit $?
		fi
	fi

	# Extract upstream if needed
	if [ ! -d "$externalPath/upstream.original" ]; then
		echo "Extracting '$externalName' upstream..."
		mkdir -p "$externalPath/upstream.tmp"
		unzip -q "$externalPath/upstream.pack" -d "$externalPath/upstream.tmp"
		if [ $? -ne 0 ]; then
			echo "Failed to extract '$externalName' upstream, aborting..."
			rm -rf "$externalPath/upstream.pack" "$externalPath/upstream.tmp" "$externalPath/upstream.original"
			exit $?
		fi
		mv "$externalPath/upstream.tmp"/* "$externalPath/upstream.original"
		rm -rf "$externalPath/upstream.tmp"
	fi
}
export -f configureExternalFromZipArchive

# Function: patchExternal(location)
patchExternal()
{
	local externalDir=$1
	local externalPath="$( cd "$externalDir" && pwd )"
	local externalName="$(basename "$externalPath")"

	echo "Looking for '$externalName' patches..."

	cp -rfp "$externalPath/upstream.original" "$externalPath/upstream.patched"
	if [ -d "$externalPath/patches" ]; then
		echo "Patching '$externalName'..."
		local patchFiles=`ls -1 "$externalPath/patches"/*.patch | sort`
		for patchFile in $patchFiles
		do
			echo "Applying "`basename $patchFile`
			patch --strip=1 --directory="$externalPath/upstream.patched/" --input="$patchFile"
			if [ $? -ne 0 ]; then
				echo "Failed to apply '$patchFile' to upstream, aborting..."
				rm -rf "$externalPath/upstream.patched"
				exit $?
			fi
		done
	fi
}
