<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="20008000">
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="helloworld_client" Type="Folder">
			<Item Name="helloworld_client.lvlib" Type="Library" URL="../helloworld_client/helloworld_client.lvlib"/>
		</Item>
		<Item Name="helloworld_server" Type="Folder">
			<Item Name="helloworld_server.lvlib" Type="Library" URL="../helloworld_server/helloworld_server.lvlib"/>
		</Item>
		<Item Name="Dependencies" Type="Dependencies">
			<Item Name="vi.lib" Type="Folder">
				<Item Name="1D String Array to Delimited String.vi" Type="VI" URL="/&lt;vilib&gt;/AdvancedString/1D String Array to Delimited String.vi"/>
				<Item Name="Any.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Shared/Any.ctl"/>
				<Item Name="Client Unary Call.vim" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Client API/Client Unary Call.vim"/>
				<Item Name="Close Client.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Client API/Close Client.vi"/>
				<Item Name="Complete Call.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Message Requests/Complete Call.vi"/>
				<Item Name="CompleteMetadataRegistration.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/CompleteMetadataRegistration.vi"/>
				<Item Name="Create And Register Server Event.vim" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Server/Create And Register Server Event.vim"/>
				<Item Name="Create Client.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Client API/Create Client.vi"/>
				<Item Name="Create Server.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/Servicer/Server API/Create Server.vi"/>
				<Item Name="Error Cluster From Error Code.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Error Cluster From Error Code.vi"/>
				<Item Name="Get gRPC ID.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/Servicer/Accessors/gRPC ID/Get gRPC ID.vi"/>
				<Item Name="Get Server DLL Path.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Server/Get Server DLL Path.vi"/>
				<Item Name="gRPCId Cluster UE.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/typeDefs/gRPCId Cluster UE.ctl"/>
				<Item Name="gRPCId Cluster.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/typeDefs/gRPCId Cluster.ctl"/>
				<Item Name="grpcId.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Shared/grpcId.ctl"/>
				<Item Name="Message Element Metadata.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/typeDefs/Message Element Metadata.ctl"/>
				<Item Name="Message Element Type.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/typeDefs/Message Element Type.ctl"/>
				<Item Name="Message Metadata.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/typeDefs/Message Metadata.ctl"/>
				<Item Name="Read Call Request.vim" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Message Requests/Read Call Request.vim"/>
				<Item Name="Register Message Metadata.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Server/Register Message Metadata.vi"/>
				<Item Name="ServiceBase.lvclass" Type="LVClass" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/ServiceBase/ServiceBase.lvclass"/>
				<Item Name="Servicer.ctl" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/Servicer/Servicer.lvclass/Servicer.ctl"/>
				<Item Name="Start Server.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/Servicer/Server API/Start Server.vi"/>
				<Item Name="Stop Server.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Servicer/Servicer/Server API/Stop Server.vi"/>
				<Item Name="TranslateGrpcError.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Shared/TranslateGrpcError.vi"/>
				<Item Name="Wait On Occurence.vi" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Shared/Wait On Occurence.vi"/>
				<Item Name="Write Call Response.vim" Type="VI" URL="/&lt;vilib&gt;/gRPC/LabVIEW gRPC Library/Server API/Message Requests/Write Call Response.vim"/>
			</Item>
		</Item>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
</Project>
