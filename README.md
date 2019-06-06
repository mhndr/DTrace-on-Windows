# DTrace on Windows 

Here at Microsoft, we are always looking to engage with open source communities to produce better solutions for the community and our customers . One of the more useful debugging advances that have arrived in the last decade is DTrace. DTrace of course needs no introduction: it’s a dynamic tracing framework that allows an admin or developer to get a real-time look into a system either in user or kernel mode. 

DTrace has a C-style high level and powerful programming language that allows you to dynamically insert trace points. Using these dynamically inserted trace points, you can filter on conditions or errors, write code to analyze lock patterns, detect deadlocks, etc. ETW while powerful, is static and does not provide the ability to programmatically insert trace points at runtime.  

There are a lot of websites and resources from the community to learn about DTrace. One of the most comprehensive one is the Dynamic Tracing Guide html book available on dtrace.org website. This ebook describes DTrace in detail and is the authoritative guide for DTrace. We also have Windows specific examples below which will provide more info. 

Starting in 2016, the OpenDTrace effort began on GitHub that  tried to ensure a portable implementation of DTrace for different operating systems. We decided to add support for DTrace on Windows using this OpenDTrace port. This is a fork of the 'opendtrace' repository and contains the unified, cross platform, source code for the OpenDTrace system including kernel components. 

# More Information

Introductory blog with install instructions and samples: https://techcommunity.microsoft.com/t5/Windows-Kernel-Internals/DTrace-on-Windows/ba-p/362902 

# Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

# License

OpenDTrace is under the CDDL license, see the `LICENSE` file in this
repository for details.

