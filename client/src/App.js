import React, { Component } from "react";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import faBars from "@fortawesome/fontawesome-free-solid/faBars";
import "./App.css";
import axios from "axios";

class InfoDisplay extends Component {
  render() {
    return <div className="displayer">{this.props.text}</div>;
  }
}

class Message extends Component {
  render() {
    return (
      <div className="message">{`${this.props.user}: ${this.props.text}`}</div>
    );
  }
}

class TopMenu extends Component {
  constructor(props) {
    super(props);
    this.state = {
      menu_class: "",
    };
  }

  setToggleTopMenuClass = () => {
    this.setState({
      menu_class: this.state.menu_class === "" ? "toggled" : "",
    });
  };

  render() {
    let top_menu_class = `top-menu ${this.state.menu_class}`;

    return (
      <div>
        <div className={top_menu_class}>
          <JoinForm pushUpdate={this.props.pushUpdate} />
          <CreateForm pushUpdate={this.props.pushUpdate} />
          <FontAwesomeIcon
            icon={faBars}
            className="top-menu-icon"
            onClick={this.setToggleTopMenuClass}
          />
          <div className="clear-fix" />
        </div>
      </div>
    );
  }
}

class MessageForm extends React.Component {
  constructor(props) {
    super(props);

    this.handleChange = this.handleChange.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
    this.state = {
      value: "",
    };

  }

  handleChange(event) {
    this.setState({ value: event.target.value });
  }

  async handleSubmit() {
    await axios.get("http://159.203.189.240:3334/send", {
      params: {
        code: this.props.code,
        user: this.props.username,
        msg: this.state.value,
      },
    });
    console.log(this.state.value);
    document.getElementById("mes").value = "";
    //this.props.addMessage(this.state.value);
  }
  handleKeyDown = ({ key }) => {
    if (key === "Enter") {
      this.handleSubmit();
    }
  };
  render() {
    return (
      <div>
        <input
          type="text"
          id="mes"
          onKeyDown={this.handleKeyDown}
          placeholder="Message... (Max length 40 characters)"
          onChange={this.handleChange}
        />
        <button onClick={this.handleSubmit}>Send</button>
      </div>
    );
  }
}

class JoinForm extends React.Component {
  constructor(props) {
    super(props);

    this.handleChange = this.handleChange.bind(this);
    this.handleChangeTwo = this.handleChangeTwo.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
    this.state = {
      value: ""
    };
  }
  handleChangeTwo(event) {
    this.setState({ valueTwo: event.target.value });
  }
  handleChange(event) {
    this.setState({ value: event.target.value });
  }

  async handleSubmit(event) {
    if(this.state.value != ""){
      const { data } = await axios.get("http://159.203.189.240:3334/join", {
        params: { code: this.state.value },
      });
      //if(data.name != undefined || data.type != undefined){
        this.props.pushUpdate({
          name: data.name,
          type: data.type,
          code: this.state.value,
        });
    //}
    }
  }

  render() {
    return (
      <div className="createForm">
        <label>Join Chat Room:</label>
        <input
          type="text"
          placeholder="Enter Private Room Code"
          onChange={this.handleChange}
        />
        <button onClick={this.handleSubmit}>Join</button>
      </div>
    );
  }
}

class CreateForm extends React.Component {
  constructor(props) {
    super(props);

    this.handleChange = this.handleChange.bind(this);
    this.handleChangeTwo = this.handleChangeTwo.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
    this.state = {
      value: ""
    }
  }

  handleChange(event) {
    this.setState({ value: event.target.value });
  }
  handleChangeTwo(event) {
    this.setState({ valueTwo: event.target.value });
  }
  async handleSubmit(event) {
    if(this.state.value != ""){
      const { data } = await axios.get("http://159.203.189.240:3334/create", {
        params: {
          type: "private",
          user: "",
          name: this.state.value,
        },
      });
      //if(data.name != undefined || data.type != undefined){
        this.props.pushUpdate({
          name: this.state.value,
          type: "private",
          code: data.code,
        });
      //}
    }
  }

  render() {
    return (
      <div className="createForm">
        <label>Create Chat Room:</label>
        <input
          type="text"
          placeholder="Enter Chat Room Name"
          onChange={this.handleChange}
        />

        <button onClick={this.handleSubmit}>Create</button>
      </div>
    );
  }
}

export default class Untitled extends Component {
  constructor() {
    super();
    this.state = {
      messages: [],
      code: 0,
      name: "",
      dead: false,
    };
  }

  updateData = async () => {
      try{
        this.getserverInfo();
        const {
          data: { messages },
        } = await axios.get("http://159.203.189.240:3334/update", {
          params: { code: this.state.code },
        });
        this.state.dead = false;
        if(messages){
          this.setState({ messages });
          return 1;
        }
        else{
          
          return 0
        }
      }
      catch(e){
        
        
        if(this.state.dead === false){
          this.state.dead = true;
          this.setState({code: 0})
          this.setState({name: ""})
          window.confirm("Server has Shutdown");

        }
      }

      
  };
  getserverInfo = async () => {
    const {
      data: { name},
    } = await axios.get("http://159.203.189.240:3334/join", {
      params: { code: this.state.code },
    });
    this.setState({ name });
  }
  componentDidMount() {
    var username = prompt("Please enter your username", "");
    if(username == null || username == ""){
      username = "John Doe"
    }
    
    this.setState({ username });

    this.getserverInfo();
    this.updateData();
    //var intervalid = setInterval(this.updateData, 1000);
    var intervalId = setInterval(() => {
      let check = this.updateData();
      if(check == 0){
       clearInterval(intervalId)
       alert ("Server has shut down!");
     }
   
   }, 1000);

  }

  updateState(values) {
    this.setState(values);
  }
  addMessage = (value) => {
    var joined = this.state.messages.concat({
      username: this.state.username,
      message: value,
    });
    this.setState({ messages: joined });
  };
  render() {
    const MyComponent = () => {
      return <InfoDisplay text={this.state.name}></InfoDisplay>;
    };

    const HiThere = () => {
      return <InfoDisplay text={this.state.code}></InfoDisplay>;
    };

    return (
      <div className="wrapper">
        <TopMenu pushUpdate={(u) => this.updateState(u)} />

        <div className="App">
          <div className="appHandler">
            <InfoDisplay text={this.state.name.replaceAll("+", " ")}></InfoDisplay>
            <InfoDisplay text={"Server Code: "+this.state.code}></InfoDisplay>

            {this.state.messages.map((item) => (
              <Message
                user={decodeURIComponent(item.username).replaceAll("+", " ")}
                text={decodeURIComponent(item.message).replaceAll("+", " ")}
              ></Message>
            ))}
          </div>
          <header className="App-header" valign="bottom">
            <MessageForm
              code={this.state.code}
              username={this.state.username}
              addMessage={this.addMessage}
            />
          </header>
        </div>
      </div>
    );
  }
}
