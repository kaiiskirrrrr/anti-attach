I have gone through x64dbgs source code and looked at their attachment process, i see they use ``CheckRemoteDebuggerPresent`` to make sure your program isnt already being debugged, if it is it throws a error and doesnt let you attach. Meaning if we exploit this and manipulate our process information to make x64dbg think it is already debugging your program, x64dbg wont be able to attach.

## how to implement
1. include attachment.hxx
2. call the "start" function at the start of your program, example below:
```cpp
auto main(int argc, char* argv[]) -> int
{
	c_already_debugged->start(argc, argv);

	printf("attachment disabled\n");
	std::cin.get();

	return 0;
}
```

### showcase:

https://github.com/user-attachments/assets/13582dc6-6acb-42e8-8816-49121975d674


